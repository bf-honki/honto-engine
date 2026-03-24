#include "honto/Level.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace
{
    std::string TrimCopy(const std::string& text)
    {
        const auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char character)
        {
            return std::isspace(character) != 0;
        });
        const auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char character)
        {
            return std::isspace(character) != 0;
        }).base();

        if (begin >= end)
        {
            return {};
        }

        return std::string(begin, end);
    }

    bool ParseVec2(const std::string& text, honto::Vec2& value)
    {
        std::stringstream stream(text);
        char comma = '\0';
        if (!(stream >> value.x >> comma >> value.y) || comma != ',')
        {
            return false;
        }

        return true;
    }

    bool ParseColor(const std::string& text, honto::Color& color)
    {
        std::stringstream stream(text);
        int r = 255;
        int g = 255;
        int b = 255;
        int a = 255;
        char comma1 = '\0';
        char comma2 = '\0';
        char comma3 = '\0';

        if (!(stream >> r >> comma1 >> g >> comma2 >> b >> comma3 >> a))
        {
            return false;
        }

        if (comma1 != ',' || comma2 != ',' || comma3 != ',')
        {
            return false;
        }

        color.r = static_cast<std::uint8_t>(std::clamp(r, 0, 255));
        color.g = static_cast<std::uint8_t>(std::clamp(g, 0, 255));
        color.b = static_cast<std::uint8_t>(std::clamp(b, 0, 255));
        color.a = static_cast<std::uint8_t>(std::clamp(a, 0, 255));
        return true;
    }

    void WriteEntity(std::ofstream& stream, const honto::LevelEntity& entity)
    {
        stream << "[entity]\n";
        stream << "name=" << entity.name << '\n';
        stream << "kind=" << entity.kind << '\n';
        stream << "position=" << entity.position.x << ',' << entity.position.y << '\n';
        stream << "size=" << entity.size.x << ',' << entity.size.y << '\n';
        stream << "layer=" << entity.layer << '\n';
        stream << "color="
               << static_cast<int>(entity.color.r) << ','
               << static_cast<int>(entity.color.g) << ','
               << static_cast<int>(entity.color.b) << ','
               << static_cast<int>(entity.color.a) << '\n';
        stream << "text=" << entity.text << '\n';
        stream << "asset=" << entity.asset << '\n';
        stream << "[/entity]\n";
    }
}

namespace honto
{
    bool LevelFile::Save(const std::string& path, const LevelDocument& level)
    {
        std::ofstream stream(path, std::ios::binary);
        if (!stream)
        {
            return false;
        }

        stream << "title=" << level.title << '\n';
        stream << "tile_size=" << level.tileSize.x << ',' << level.tileSize.y << '\n';
        stream << "[map]\n";
        for (const std::string& row : level.map)
        {
            stream << row << '\n';
        }
        stream << "[/map]\n";

        for (const LevelEntity& entity : level.entities)
        {
            WriteEntity(stream, entity);
        }

        return static_cast<bool>(stream);
    }

    bool LevelFile::Load(const std::string& path, LevelDocument& level)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            return false;
        }

        level = {};

        std::string line;
        bool readingMap = false;
        bool readingEntity = false;
        LevelEntity currentEntity;

        while (std::getline(stream, line))
        {
            const std::string trimmed = TrimCopy(line);
            if (trimmed.empty())
            {
                continue;
            }

            if (trimmed == "[map]")
            {
                readingMap = true;
                continue;
            }

            if (trimmed == "[/map]")
            {
                readingMap = false;
                continue;
            }

            if (trimmed == "[entity]")
            {
                readingEntity = true;
                currentEntity = {};
                continue;
            }

            if (trimmed == "[/entity]")
            {
                readingEntity = false;
                level.entities.push_back(currentEntity);
                continue;
            }

            if (readingMap)
            {
                level.map.push_back(line);
                continue;
            }

            const std::size_t separator = trimmed.find('=');
            if (separator == std::string::npos)
            {
                continue;
            }

            const std::string key = TrimCopy(trimmed.substr(0, separator));
            const std::string value = trimmed.substr(separator + 1);

            if (readingEntity)
            {
                if (key == "name")
                {
                    currentEntity.name = value;
                }
                else if (key == "kind")
                {
                    currentEntity.kind = value;
                }
                else if (key == "position")
                {
                    ParseVec2(value, currentEntity.position);
                }
                else if (key == "size")
                {
                    ParseVec2(value, currentEntity.size);
                }
                else if (key == "layer")
                {
                    currentEntity.layer = std::stoi(value);
                }
                else if (key == "color")
                {
                    ParseColor(value, currentEntity.color);
                }
                else if (key == "text")
                {
                    currentEntity.text = value;
                }
                else if (key == "asset")
                {
                    currentEntity.asset = value;
                }

                continue;
            }

            if (key == "title")
            {
                level.title = value;
            }
            else if (key == "tile_size")
            {
                ParseVec2(value, level.tileSize);
            }
        }

        return level.IsValid();
    }

    const LevelEntity* FindLevelEntity(const LevelDocument& level, const std::string& name)
    {
        const auto found = std::find_if(
            level.entities.begin(),
            level.entities.end(),
            [&name](const LevelEntity& entity)
            {
                return entity.name == name;
            }
        );

        if (found == level.entities.end())
        {
            return nullptr;
        }

        return &(*found);
    }
}
