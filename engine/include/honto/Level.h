#pragma once

#include "Color.h"
#include "Math.h"

#include <string>
#include <vector>

namespace honto
{
    struct LevelEntity
    {
        std::string name;
        std::string kind;
        Vec2 position {};
        Vec2 size {};
        int layer = 0;
        Color color { 255, 255, 255, 255 };
        std::string text;
        std::string asset;
    };

    struct LevelDocument
    {
        std::string title;
        Vec2 tileSize { 16.0f, 16.0f };
        std::vector<std::string> map;
        std::vector<LevelEntity> entities;

        bool IsValid() const
        {
            return !map.empty();
        }
    };

    class LevelFile
    {
    public:
        static bool Save(const std::string& path, const LevelDocument& level);
        static bool Load(const std::string& path, LevelDocument& level);
        static bool SaveJson(const std::string& path, const LevelDocument& level);
        static bool LoadJson(const std::string& path, LevelDocument& level);
    };

    const LevelEntity* FindLevelEntity(const LevelDocument& level, const std::string& name);
}
