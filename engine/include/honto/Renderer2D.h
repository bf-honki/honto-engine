#pragma once

#include "Color.h"
#include "Math.h"

#include <cstdint>
#include <string>

namespace honto
{
    class Texture;
    struct TextureRegion;

    class Renderer2D
    {
    public:
        void Attach(std::uint32_t* pixels, int width, int height);
        void BeginFrame(Color clearColor);
        void SetCamera(const Vec2& position, float zoom = 1.0f);
        void ResetCamera();
        Vec2 GetCameraPosition() const;
        float GetCameraZoom() const;
        void DrawFilledRect(const Vec2& position, const Vec2& size, Color color, bool useCamera = true);
        void DrawRectOutline(const Vec2& position, const Vec2& size, Color color, int thickness = 1, bool useCamera = true);
        void DrawTexturedRect(const Vec2& position, const Vec2& size, const Texture& texture, Color tint = { 255, 255, 255, 255 }, bool useCamera = true);
        void DrawTexturedRectRegion(
            const Vec2& position,
            const Vec2& size,
            const Texture& texture,
            const TextureRegion& region,
            Color tint = { 255, 255, 255, 255 },
            bool useCamera = true
        );
        void DrawTexturedColumn(
            int x,
            float top,
            float bottom,
            const Texture& texture,
            float u,
            Color tint = { 255, 255, 255, 255 },
            bool useCamera = false
        );
        Vec2 MeasureText(const std::string& text, int pixelHeight) const;
        void DrawText(
            const std::string& text,
            const Vec2& position,
            int pixelHeight,
            Color color = { 255, 255, 255, 255 },
            bool useCamera = false
        );

        int Width() const
        {
            return m_Width;
        }

        int Height() const
        {
            return m_Height;
        }

    private:
        Vec2 TransformPosition(const Vec2& position, bool useCamera) const;
        Vec2 TransformSize(const Vec2& size, bool useCamera) const;
        void PutPixel(int x, int y, Color color);
        static std::uint32_t PackColor(Color color);
        static Color UnpackColor(std::uint32_t color);

        std::uint32_t* m_Pixels = nullptr;
        int m_Width = 0;
        int m_Height = 0;
        Vec2 m_CameraPosition {};
        float m_CameraZoom = 1.0f;
    };
}
