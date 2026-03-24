#pragma once

#include "Color.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace honto
{
    struct TextureRegion
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;

        bool IsValid() const
        {
            return width > 0 && height > 0;
        }
    };

    class Texture
    {
    public:
        bool LoadBmp(const std::string& path);

        int Width() const
        {
            return m_Width;
        }

        int Height() const
        {
            return m_Height;
        }

        bool IsValid() const
        {
            return m_Width > 0 && m_Height > 0 && !m_Pixels.empty();
        }

        Color Sample(float u, float v) const;
        Color SampleRegion(float u, float v, const TextureRegion& region) const;

        static std::shared_ptr<Texture> LoadBmpShared(const std::string& path);
        static std::shared_ptr<Texture> CreateCheckerboard(
            int width,
            int height,
            Color a,
            Color b,
            int cellSize = 8
        );
        static std::shared_ptr<Texture> CreateFrameSheet(
            int frameWidth,
            int frameHeight,
            const std::vector<Color>& frameColors,
            int columns = 0
        );

    private:
        int m_Width = 0;
        int m_Height = 0;
        std::vector<Color> m_Pixels;
    };
}
