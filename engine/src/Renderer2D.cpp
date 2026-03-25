#include "honto/Renderer2D.h"

#include "honto/Texture.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#ifdef DrawText
#undef DrawText
#endif

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace
{
    std::wstring Utf8ToWide(const std::string& text)
    {
        if (text.empty())
        {
            return {};
        }

        const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        if (length <= 1)
        {
            return {};
        }

        std::wstring result(static_cast<std::size_t>(length - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
        return result;
    }

    std::vector<std::wstring> SplitLines(const std::wstring& text)
    {
        std::vector<std::wstring> lines;
        std::wstring current;

        for (wchar_t character : text)
        {
            if (character == L'\n')
            {
                lines.push_back(current);
                current.clear();
                continue;
            }

            current.push_back(character);
        }

        lines.push_back(current);
        return lines;
    }

    HFONT CreateUiFont(int pixelHeight)
    {
        const int safeHeight = std::max(8, pixelHeight);
        return CreateFontW(
            -safeHeight,
            0,
            0,
            0,
            FW_MEDIUM,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Malgun Gothic"
        );
    }
}

namespace honto
{
    void Renderer2D::Attach(std::uint32_t* pixels, int width, int height)
    {
        m_Pixels = pixels;
        m_Width = width;
        m_Height = height;
    }

    void Renderer2D::SetCamera(const Vec2& position, float zoom)
    {
        m_CameraPosition = position;
        m_CameraZoom = std::max(0.01f, zoom);
    }

    void Renderer2D::ResetCamera()
    {
        m_CameraPosition = {};
        m_CameraZoom = 1.0f;
    }

    Vec2 Renderer2D::GetCameraPosition() const
    {
        return m_CameraPosition;
    }

    float Renderer2D::GetCameraZoom() const
    {
        return m_CameraZoom;
    }

    void Renderer2D::BeginFrame(Color clearColor)
    {
        if (m_Pixels == nullptr || m_Width <= 0 || m_Height <= 0)
        {
            return;
        }

        const std::uint32_t packed = PackColor(clearColor);
        std::fill(m_Pixels, m_Pixels + (m_Width * m_Height), packed);
    }

    void Renderer2D::DrawFilledRect(const Vec2& position, const Vec2& size, Color color, bool useCamera)
    {
        const Vec2 screenPosition = TransformPosition(position, useCamera);
        const Vec2 screenSize = TransformSize(size, useCamera);
        const int startX = std::max(0, static_cast<int>(std::floor(screenPosition.x)));
        const int startY = std::max(0, static_cast<int>(std::floor(screenPosition.y)));
        const int endX = std::min(m_Width, static_cast<int>(std::ceil(screenPosition.x + screenSize.x)));
        const int endY = std::min(m_Height, static_cast<int>(std::ceil(screenPosition.y + screenSize.y)));

        for (int y = startY; y < endY; ++y)
        {
            for (int x = startX; x < endX; ++x)
            {
                PutPixel(x, y, color);
            }
        }
    }

    void Renderer2D::DrawRectOutline(const Vec2& position, const Vec2& size, Color color, int thickness, bool useCamera)
    {
        const int border = std::max(1, thickness);
        DrawFilledRect(position, { size.x, static_cast<float>(border) }, color, useCamera);
        DrawFilledRect({ position.x, position.y + size.y - border }, { size.x, static_cast<float>(border) }, color, useCamera);
        DrawFilledRect(position, { static_cast<float>(border), size.y }, color, useCamera);
        DrawFilledRect({ position.x + size.x - border, position.y }, { static_cast<float>(border), size.y }, color, useCamera);
    }

    void Renderer2D::DrawTexturedRect(const Vec2& position, const Vec2& size, const Texture& texture, Color tint, bool useCamera)
    {
        DrawTexturedRectRegion(position, size, texture, {}, tint, useCamera);
    }

    void Renderer2D::DrawTexturedRectRegion(
        const Vec2& position,
        const Vec2& size,
        const Texture& texture,
        const TextureRegion& region,
        Color tint,
        bool useCamera
    )
    {
        if (!texture.IsValid())
        {
            DrawFilledRect(position, size, tint, useCamera);
            return;
        }

        const Vec2 screenPosition = TransformPosition(position, useCamera);
        const Vec2 screenSize = TransformSize(size, useCamera);
        const int startX = std::max(0, static_cast<int>(std::floor(screenPosition.x)));
        const int startY = std::max(0, static_cast<int>(std::floor(screenPosition.y)));
        const int endX = std::min(m_Width, static_cast<int>(std::ceil(screenPosition.x + screenSize.x)));
        const int endY = std::min(m_Height, static_cast<int>(std::ceil(screenPosition.y + screenSize.y)));

        const float safeWidth = std::max(1.0f, screenSize.x);
        const float safeHeight = std::max(1.0f, screenSize.y);

        for (int y = startY; y < endY; ++y)
        {
            for (int x = startX; x < endX; ++x)
            {
                const float u = (static_cast<float>(x) - screenPosition.x) / safeWidth;
                const float v = (static_cast<float>(y) - screenPosition.y) / safeHeight;
                Color sample = texture.SampleRegion(u, v, region);
                sample.r = static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.r) * tint.r) / 255);
                sample.g = static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.g) * tint.g) / 255);
                sample.b = static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.b) * tint.b) / 255);
                sample.a = static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.a) * tint.a) / 255);
                PutPixel(x, y, sample);
            }
        }
    }

    void Renderer2D::DrawTexturedColumn(
        int x,
        float top,
        float bottom,
        const Texture& texture,
        float u,
        Color tint,
        bool useCamera
    )
    {
        if (x < 0 || x >= m_Width || top >= bottom)
        {
            return;
        }

        const Vec2 transformedTop = TransformPosition({ static_cast<float>(x), top }, useCamera);
        const Vec2 transformedBottom = TransformPosition({ static_cast<float>(x), bottom }, useCamera);
        const int drawX = std::clamp(static_cast<int>(std::floor(transformedTop.x)), 0, m_Width - 1);
        const int startY = std::max(0, static_cast<int>(std::floor(std::min(transformedTop.y, transformedBottom.y))));
        const int endY = std::min(m_Height, static_cast<int>(std::ceil(std::max(transformedTop.y, transformedBottom.y))));
        const float height = std::max(1.0f, static_cast<float>(endY - startY));

        for (int y = startY; y < endY; ++y)
        {
            float v = (static_cast<float>(y - startY)) / height;
            Color sample = texture.IsValid() ? texture.Sample(u, v) : tint;
            sample.r = static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.r) * tint.r) / 255);
            sample.g = static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.g) * tint.g) / 255);
            sample.b = static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.b) * tint.b) / 255);
            sample.a = static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample.a) * tint.a) / 255);
            PutPixel(drawX, y, sample);
        }
    }

    Vec2 Renderer2D::MeasureText(const std::string& text, int pixelHeight) const
    {
        const std::wstring wideText = Utf8ToWide(text);
        const std::vector<std::wstring> lines = SplitLines(wideText);
        if (lines.empty())
        {
            return {};
        }

        HDC deviceContext = CreateCompatibleDC(nullptr);
        if (deviceContext == nullptr)
        {
            return {};
        }

        HFONT font = CreateUiFont(pixelHeight);
        HGDIOBJ oldFont = SelectObject(deviceContext, font);

        TEXTMETRICW metrics {};
        GetTextMetricsW(deviceContext, &metrics);

        int width = 0;
        int height = 0;
        for (const std::wstring& line : lines)
        {
            SIZE extent {};
            if (!line.empty())
            {
                GetTextExtentPoint32W(deviceContext, line.c_str(), static_cast<int>(line.size()), &extent);
            }

            width = std::max(width, static_cast<int>(extent.cx));
            height += static_cast<int>(metrics.tmHeight + metrics.tmExternalLeading);
        }

        SelectObject(deviceContext, oldFont);
        DeleteObject(font);
        DeleteDC(deviceContext);
        return {
            static_cast<float>(width),
            static_cast<float>(std::max(0, height))
        };
    }

    void Renderer2D::DrawText(const std::string& text, const Vec2& position, int pixelHeight, Color color, bool useCamera)
    {
        if (m_Pixels == nullptr || m_Width <= 0 || m_Height <= 0 || color.a == 0)
        {
            return;
        }

        const std::wstring wideText = Utf8ToWide(text);
        const std::vector<std::wstring> lines = SplitLines(wideText);
        if (lines.empty())
        {
            return;
        }

        HDC deviceContext = CreateCompatibleDC(nullptr);
        if (deviceContext == nullptr)
        {
            return;
        }

        HFONT font = CreateUiFont(pixelHeight);
        HGDIOBJ oldFont = SelectObject(deviceContext, font);

        TEXTMETRICW metrics {};
        GetTextMetricsW(deviceContext, &metrics);
        const int lineHeight = std::max(1, static_cast<int>(metrics.tmHeight + metrics.tmExternalLeading));

        int bitmapWidth = 0;
        for (const std::wstring& line : lines)
        {
            SIZE extent {};
            if (!line.empty())
            {
                GetTextExtentPoint32W(deviceContext, line.c_str(), static_cast<int>(line.size()), &extent);
            }
            bitmapWidth = std::max(bitmapWidth, static_cast<int>(extent.cx));
        }

        const int bitmapHeight = std::max(1, lineHeight * static_cast<int>(lines.size()));
        bitmapWidth = std::max(1, bitmapWidth);

        BITMAPINFO bitmapInfo {};
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = bitmapWidth;
        bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HBITMAP bitmap = CreateDIBSection(deviceContext, &bitmapInfo, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (bitmap == nullptr || bits == nullptr)
        {
            SelectObject(deviceContext, oldFont);
            DeleteObject(font);
            DeleteDC(deviceContext);
            return;
        }

        HGDIOBJ oldBitmap = SelectObject(deviceContext, bitmap);
        std::fill_n(static_cast<std::uint32_t*>(bits), static_cast<std::size_t>(bitmapWidth * bitmapHeight), 0u);

        SetBkMode(deviceContext, TRANSPARENT);
        SetTextColor(deviceContext, RGB(255, 255, 255));

        int y = 0;
        for (const std::wstring& line : lines)
        {
            if (!line.empty())
            {
                TextOutW(deviceContext, 0, y, line.c_str(), static_cast<int>(line.size()));
            }
            y += lineHeight;
        }

        const Vec2 screenPosition = TransformPosition(position, useCamera);
        const std::uint8_t textAlpha = color.a;
        auto* sourcePixels = static_cast<std::uint32_t*>(bits);

        for (int row = 0; row < bitmapHeight; ++row)
        {
            for (int column = 0; column < bitmapWidth; ++column)
            {
                const std::uint32_t sample = sourcePixels[(row * bitmapWidth) + column];
                const std::uint8_t blue = static_cast<std::uint8_t>(sample & 0xFF);
                const std::uint8_t green = static_cast<std::uint8_t>((sample >> 8) & 0xFF);
                const std::uint8_t red = static_cast<std::uint8_t>((sample >> 16) & 0xFF);
                const std::uint8_t coverage = std::max(red, std::max(green, blue));
                if (coverage == 0)
                {
                    continue;
                }

                Color pixelColor = color;
                pixelColor.a = static_cast<std::uint8_t>((static_cast<std::uint32_t>(coverage) * textAlpha) / 255);
                PutPixel(
                    static_cast<int>(std::round(screenPosition.x)) + column,
                    static_cast<int>(std::round(screenPosition.y)) + row,
                    pixelColor
                );
            }
        }

        SelectObject(deviceContext, oldBitmap);
        SelectObject(deviceContext, oldFont);
        DeleteObject(bitmap);
        DeleteObject(font);
        DeleteDC(deviceContext);
    }

    Vec2 Renderer2D::TransformPosition(const Vec2& position, bool useCamera) const
    {
        if (!useCamera)
        {
            return position;
        }

        return {
            (position.x - m_CameraPosition.x) * m_CameraZoom,
            (position.y - m_CameraPosition.y) * m_CameraZoom
        };
    }

    Vec2 Renderer2D::TransformSize(const Vec2& size, bool useCamera) const
    {
        if (!useCamera)
        {
            return size;
        }

        return size * m_CameraZoom;
    }

    void Renderer2D::PutPixel(int x, int y, Color color)
    {
        if (m_Pixels == nullptr)
        {
            return;
        }

        if (x < 0 || y < 0 || x >= m_Width || y >= m_Height)
        {
            return;
        }

        const int index = (y * m_Width) + x;
        if (color.a >= 255)
        {
            m_Pixels[index] = PackColor(color);
            return;
        }

        if (color.a == 0)
        {
            return;
        }

        const Color destination = UnpackColor(m_Pixels[index]);
        const std::uint32_t alpha = color.a;
        const std::uint32_t inverseAlpha = 255 - alpha;

        Color blended {};
        blended.r = static_cast<std::uint8_t>(((static_cast<std::uint32_t>(color.r) * alpha) + (static_cast<std::uint32_t>(destination.r) * inverseAlpha)) / 255);
        blended.g = static_cast<std::uint8_t>(((static_cast<std::uint32_t>(color.g) * alpha) + (static_cast<std::uint32_t>(destination.g) * inverseAlpha)) / 255);
        blended.b = static_cast<std::uint8_t>(((static_cast<std::uint32_t>(color.b) * alpha) + (static_cast<std::uint32_t>(destination.b) * inverseAlpha)) / 255);
        blended.a = 255;
        m_Pixels[index] = PackColor(blended);
    }

    std::uint32_t Renderer2D::PackColor(Color color)
    {
        return (static_cast<std::uint32_t>(color.a) << 24)
            | (static_cast<std::uint32_t>(color.r) << 16)
            | (static_cast<std::uint32_t>(color.g) << 8)
            | static_cast<std::uint32_t>(color.b);
    }

    Color Renderer2D::UnpackColor(std::uint32_t color)
    {
        return {
            static_cast<std::uint8_t>((color >> 16) & 0xFF),
            static_cast<std::uint8_t>((color >> 8) & 0xFF),
            static_cast<std::uint8_t>(color & 0xFF),
            static_cast<std::uint8_t>((color >> 24) & 0xFF)
        };
    }
}
