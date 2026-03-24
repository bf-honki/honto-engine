#pragma once

#include "Color.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace honto
{
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

        static std::shared_ptr<Texture> LoadBmpShared(const std::string& path);
        static std::shared_ptr<Texture> CreateCheckerboard(
            int width,
            int height,
            Color a,
            Color b,
            int cellSize = 8
        );

    private:
        int m_Width = 0;
        int m_Height = 0;
        std::vector<Color> m_Pixels;
    };
}
