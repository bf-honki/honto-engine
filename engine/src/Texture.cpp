#include "honto/Texture.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>

namespace
{
    std::uint16_t ReadUInt16(std::ifstream& stream)
    {
        std::uint8_t bytes[2] {};
        stream.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
        return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
    }

    std::uint32_t ReadUInt32(std::ifstream& stream)
    {
        std::uint8_t bytes[4] {};
        stream.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
        return static_cast<std::uint32_t>(bytes[0])
            | (static_cast<std::uint32_t>(bytes[1]) << 8)
            | (static_cast<std::uint32_t>(bytes[2]) << 16)
            | (static_cast<std::uint32_t>(bytes[3]) << 24);
    }

    std::int32_t ReadInt32(std::ifstream& stream)
    {
        return static_cast<std::int32_t>(ReadUInt32(stream));
    }

    std::wstring ToWide(const std::string& text)
    {
        if (text.empty())
        {
            return {};
        }

        const int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        if (required <= 0)
        {
            return {};
        }

        std::wstring wide(static_cast<std::size_t>(required), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), required);

        if (!wide.empty() && wide.back() == L'\0')
        {
            wide.pop_back();
        }

        return wide;
    }

    std::string ToLowerCopy(std::string text)
    {
        for (char& character : text)
        {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }

        return text;
    }

    class GdiplusRuntime
    {
    public:
        GdiplusRuntime()
        {
            Gdiplus::GdiplusStartupInput input;
            m_Status = Gdiplus::GdiplusStartup(&m_Token, &input, nullptr);
        }

        ~GdiplusRuntime()
        {
            if (m_Token != 0)
            {
                Gdiplus::GdiplusShutdown(m_Token);
            }
        }

        bool Ready() const
        {
            return m_Status == Gdiplus::Ok && m_Token != 0;
        }

    private:
        ULONG_PTR m_Token = 0;
        Gdiplus::Status m_Status = Gdiplus::GenericError;
    };

    GdiplusRuntime& GetGdiplusRuntime()
    {
        static GdiplusRuntime runtime;
        return runtime;
    }
}

namespace honto
{
    bool Texture::Load(const std::string& path)
    {
        const std::string lowerPath = ToLowerCopy(path);
        if (lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size() - 4) == ".bmp")
        {
            return LoadBmp(path);
        }

        if (lowerPath.size() >= 4 && lowerPath.substr(lowerPath.size() - 4) == ".png")
        {
            return LoadPng(path);
        }

        return LoadBmp(path) || LoadPng(path);
    }

    bool Texture::LoadBmp(const std::string& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            return false;
        }

        const std::uint16_t signature = ReadUInt16(stream);
        if (signature != 0x4D42)
        {
            return false;
        }

        (void)ReadUInt32(stream);
        (void)ReadUInt16(stream);
        (void)ReadUInt16(stream);
        const std::uint32_t pixelOffset = ReadUInt32(stream);
        const std::uint32_t dibHeaderSize = ReadUInt32(stream);
        if (dibHeaderSize < 40)
        {
            return false;
        }

        const std::int32_t width = ReadInt32(stream);
        const std::int32_t height = ReadInt32(stream);
        const std::uint16_t planes = ReadUInt16(stream);
        const std::uint16_t bitsPerPixel = ReadUInt16(stream);
        const std::uint32_t compression = ReadUInt32(stream);
        (void)ReadUInt32(stream);
        (void)ReadInt32(stream);
        (void)ReadInt32(stream);
        (void)ReadUInt32(stream);
        (void)ReadUInt32(stream);

        if (planes != 1 || compression != 0 || (bitsPerPixel != 24 && bitsPerPixel != 32) || width <= 0 || height == 0)
        {
            return false;
        }

        const bool topDown = height < 0;
        const int imageWidth = width;
        const int imageHeight = std::abs(height);
        const int bytesPerPixel = bitsPerPixel / 8;
        const int rowStride = ((imageWidth * bytesPerPixel) + 3) & ~3;

        m_Width = imageWidth;
        m_Height = imageHeight;
        m_Pixels.assign(static_cast<std::size_t>(m_Width * m_Height), {});

        stream.seekg(static_cast<std::streamoff>(pixelOffset), std::ios::beg);
        if (!stream)
        {
            m_Width = 0;
            m_Height = 0;
            m_Pixels.clear();
            return false;
        }

        std::vector<std::uint8_t> row(static_cast<std::size_t>(rowStride), 0);
        for (int rowIndex = 0; rowIndex < m_Height; ++rowIndex)
        {
            stream.read(reinterpret_cast<char*>(row.data()), row.size());
            if (!stream)
            {
                m_Width = 0;
                m_Height = 0;
                m_Pixels.clear();
                return false;
            }

            const int destinationY = topDown ? rowIndex : (m_Height - 1 - rowIndex);
            for (int x = 0; x < m_Width; ++x)
            {
                const std::size_t sourceIndex = static_cast<std::size_t>(x * bytesPerPixel);
                Color color;
                color.b = row[sourceIndex + 0];
                color.g = row[sourceIndex + 1];
                color.r = row[sourceIndex + 2];
                color.a = (bytesPerPixel == 4) ? row[sourceIndex + 3] : 255;
                m_Pixels[static_cast<std::size_t>((destinationY * m_Width) + x)] = color;
            }
        }

        return true;
    }

    bool Texture::LoadPng(const std::string& path)
    {
        if (!GetGdiplusRuntime().Ready())
        {
            return false;
        }

        const std::wstring widePath = ToWide(path);
        if (widePath.empty())
        {
            return false;
        }

        Gdiplus::Bitmap bitmap(widePath.c_str());
        if (bitmap.GetLastStatus() != Gdiplus::Ok)
        {
            return false;
        }

        const int width = static_cast<int>(bitmap.GetWidth());
        const int height = static_cast<int>(bitmap.GetHeight());
        if (width <= 0 || height <= 0)
        {
            return false;
        }

        m_Width = width;
        m_Height = height;
        m_Pixels.assign(static_cast<std::size_t>(m_Width * m_Height), {});

        for (int y = 0; y < m_Height; ++y)
        {
            for (int x = 0; x < m_Width; ++x)
            {
                Gdiplus::Color pixel;
                if (bitmap.GetPixel(x, y, &pixel) != Gdiplus::Ok)
                {
                    m_Width = 0;
                    m_Height = 0;
                    m_Pixels.clear();
                    return false;
                }

                Color color;
                color.r = pixel.GetR();
                color.g = pixel.GetG();
                color.b = pixel.GetB();
                color.a = pixel.GetA();
                m_Pixels[static_cast<std::size_t>((y * m_Width) + x)] = color;
            }
        }

        return true;
    }

    Color Texture::Sample(float u, float v) const
    {
        if (!IsValid())
        {
            return {};
        }

        const float wrappedU = u - std::floor(u);
        const float wrappedV = v - std::floor(v);
        const int x = std::clamp(static_cast<int>(wrappedU * static_cast<float>(m_Width)), 0, m_Width - 1);
        const int y = std::clamp(static_cast<int>(wrappedV * static_cast<float>(m_Height)), 0, m_Height - 1);
        return m_Pixels[static_cast<std::size_t>((y * m_Width) + x)];
    }

    Color Texture::SampleRegion(float u, float v, const TextureRegion& region) const
    {
        if (!region.IsValid())
        {
            return Sample(u, v);
        }

        if (!IsValid())
        {
            return {};
        }

        const int startX = std::clamp(region.x, 0, m_Width - 1);
        const int startY = std::clamp(region.y, 0, m_Height - 1);
        const int endX = std::clamp(region.x + region.width, startX + 1, m_Width);
        const int endY = std::clamp(region.y + region.height, startY + 1, m_Height);
        const float wrappedU = u - std::floor(u);
        const float wrappedV = v - std::floor(v);
        const int x = std::clamp(
            startX + static_cast<int>(wrappedU * static_cast<float>(endX - startX)),
            startX,
            endX - 1
        );
        const int y = std::clamp(
            startY + static_cast<int>(wrappedV * static_cast<float>(endY - startY)),
            startY,
            endY - 1
        );
        return m_Pixels[static_cast<std::size_t>((y * m_Width) + x)];
    }

    std::shared_ptr<Texture> Texture::LoadShared(const std::string& path)
    {
        auto texture = std::make_shared<Texture>();
        if (texture != nullptr && texture->Load(path))
        {
            return texture;
        }

        return nullptr;
    }

    std::shared_ptr<Texture> Texture::LoadBmpShared(const std::string& path)
    {
        auto texture = std::make_shared<Texture>();
        if (texture != nullptr && texture->LoadBmp(path))
        {
            return texture;
        }

        return nullptr;
    }

    std::shared_ptr<Texture> Texture::LoadPngShared(const std::string& path)
    {
        auto texture = std::make_shared<Texture>();
        if (texture != nullptr && texture->LoadPng(path))
        {
            return texture;
        }

        return nullptr;
    }

    std::shared_ptr<Texture> Texture::CreateCheckerboard(
        int width,
        int height,
        Color a,
        Color b,
        int cellSize
    )
    {
        if (width <= 0 || height <= 0)
        {
            return nullptr;
        }

        auto texture = std::make_shared<Texture>();
        if (texture == nullptr)
        {
            return nullptr;
        }

        texture->m_Width = width;
        texture->m_Height = height;
        texture->m_Pixels.assign(static_cast<std::size_t>(width * height), {});
        const int safeCellSize = std::max(1, cellSize);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const bool usePrimary = (((x / safeCellSize) + (y / safeCellSize)) % 2) == 0;
                texture->m_Pixels[static_cast<std::size_t>((y * width) + x)] = usePrimary ? a : b;
            }
        }

        return texture;
    }

    std::shared_ptr<Texture> Texture::CreateFrameSheet(
        int frameWidth,
        int frameHeight,
        const std::vector<Color>& frameColors,
        int columns
    )
    {
        if (frameWidth <= 0 || frameHeight <= 0 || frameColors.empty())
        {
            return nullptr;
        }

        const int safeColumns = std::max(1, columns <= 0 ? static_cast<int>(frameColors.size()) : columns);
        const int rows = static_cast<int>((frameColors.size() + static_cast<std::size_t>(safeColumns) - 1) / static_cast<std::size_t>(safeColumns));

        auto texture = std::make_shared<Texture>();
        if (texture == nullptr)
        {
            return nullptr;
        }

        texture->m_Width = frameWidth * safeColumns;
        texture->m_Height = frameHeight * rows;
        texture->m_Pixels.assign(static_cast<std::size_t>(texture->m_Width * texture->m_Height), Color { 0, 0, 0, 0 });

        for (std::size_t frameIndex = 0; frameIndex < frameColors.size(); ++frameIndex)
        {
            const int column = static_cast<int>(frameIndex % static_cast<std::size_t>(safeColumns));
            const int row = static_cast<int>(frameIndex / static_cast<std::size_t>(safeColumns));
            const int originX = column * frameWidth;
            const int originY = row * frameHeight;
            const Color base = frameColors[frameIndex];

            for (int y = 0; y < frameHeight; ++y)
            {
                for (int x = 0; x < frameWidth; ++x)
                {
                    const bool border = x == 0 || y == 0 || x == frameWidth - 1 || y == frameHeight - 1;
                    const bool stripe = ((x / std::max(1, frameWidth / 4)) + static_cast<int>(frameIndex)) % 2 == 0;
                    Color pixel = base;

                    if (border)
                    {
                        pixel.r = static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.r) + 24));
                        pixel.g = static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.g) + 24));
                        pixel.b = static_cast<std::uint8_t>(std::min(255, static_cast<int>(base.b) + 24));
                    }
                    else if (stripe)
                    {
                        pixel.r = static_cast<std::uint8_t>(std::max(0, static_cast<int>(base.r) - 18));
                        pixel.g = static_cast<std::uint8_t>(std::max(0, static_cast<int>(base.g) - 18));
                        pixel.b = static_cast<std::uint8_t>(std::max(0, static_cast<int>(base.b) - 18));
                    }

                    texture->m_Pixels[static_cast<std::size_t>(((originY + y) * texture->m_Width) + (originX + x))] = pixel;
                }
            }
        }

        return texture;
    }
}
