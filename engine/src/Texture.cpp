#include "honto/Texture.h"

#include <algorithm>
#include <cmath>
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
}

namespace honto
{
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

    std::shared_ptr<Texture> Texture::LoadBmpShared(const std::string& path)
    {
        auto texture = std::make_shared<Texture>();
        if (texture != nullptr && texture->LoadBmp(path))
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
}
