#include "honto/Renderer2D.h"

#include "honto/Texture.h"

#include <algorithm>
#include <cmath>

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
                Color sample = texture.Sample(u, v);
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
