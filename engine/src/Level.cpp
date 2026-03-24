#include "honto/Level.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

    std::string ToLowerCopy(std::string text)
    {
        std::transform(
            text.begin(),
            text.end(),
            text.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            }
        );
        return text;
    }

    bool EndsWithIgnoreCase(const std::string& value, const std::string& suffix)
    {
        if (value.size() < suffix.size())
        {
            return false;
        }

        return ToLowerCopy(value.substr(value.size() - suffix.size())) == ToLowerCopy(suffix);
    }

    bool ParseVec2Text(const std::string& text, honto::Vec2& value)
    {
        std::stringstream stream(text);
        char comma = '\0';
        if (!(stream >> value.x >> comma >> value.y) || comma != ',')
        {
            return false;
        }

        return true;
    }

    bool ParseColorText(const std::string& text, honto::Color& color)
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

    struct JsonValue
    {
        enum class Type
        {
            Null,
            Number,
            String,
            Array,
            Object,
            Boolean
        };

        Type type = Type::Null;
        double numberValue = 0.0;
        bool boolValue = false;
        std::string stringValue;
        std::vector<JsonValue> arrayValue;
        std::unordered_map<std::string, JsonValue> objectValue;

        const JsonValue* Find(const std::string& key) const
        {
            const auto found = objectValue.find(key);
            if (found == objectValue.end())
            {
                return nullptr;
            }

            return &found->second;
        }
    };

    class JsonParser
    {
    public:
        explicit JsonParser(const std::string& text)
            : m_Text(text)
        {
        }

        bool Parse(JsonValue& value)
        {
            SkipWhitespace();
            if (!ParseValue(value))
            {
                return false;
            }

            SkipWhitespace();
            return m_Index == m_Text.size();
        }

    private:
        bool ParseValue(JsonValue& value)
        {
            SkipWhitespace();
            if (m_Index >= m_Text.size())
            {
                return false;
            }

            const char token = m_Text[m_Index];
            if (token == '{')
            {
                return ParseObject(value);
            }

            if (token == '[')
            {
                return ParseArray(value);
            }

            if (token == '"')
            {
                value.type = JsonValue::Type::String;
                return ParseString(value.stringValue);
            }

            if (token == '-' || std::isdigit(static_cast<unsigned char>(token)) != 0)
            {
                value.type = JsonValue::Type::Number;
                return ParseNumber(value.numberValue);
            }

            if (MatchKeyword("true"))
            {
                value.type = JsonValue::Type::Boolean;
                value.boolValue = true;
                return true;
            }

            if (MatchKeyword("false"))
            {
                value.type = JsonValue::Type::Boolean;
                value.boolValue = false;
                return true;
            }

            if (MatchKeyword("null"))
            {
                value.type = JsonValue::Type::Null;
                return true;
            }

            return false;
        }

        bool ParseObject(JsonValue& value)
        {
            if (!Consume('{'))
            {
                return false;
            }

            value.type = JsonValue::Type::Object;
            value.objectValue.clear();

            SkipWhitespace();
            if (Consume('}'))
            {
                return true;
            }

            while (m_Index < m_Text.size())
            {
                std::string key;
                if (!ParseString(key))
                {
                    return false;
                }

                SkipWhitespace();
                if (!Consume(':'))
                {
                    return false;
                }

                JsonValue item;
                if (!ParseValue(item))
                {
                    return false;
                }

                value.objectValue.emplace(std::move(key), std::move(item));
                SkipWhitespace();

                if (Consume('}'))
                {
                    return true;
                }

                if (!Consume(','))
                {
                    return false;
                }
            }

            return false;
        }

        bool ParseArray(JsonValue& value)
        {
            if (!Consume('['))
            {
                return false;
            }

            value.type = JsonValue::Type::Array;
            value.arrayValue.clear();

            SkipWhitespace();
            if (Consume(']'))
            {
                return true;
            }

            while (m_Index < m_Text.size())
            {
                JsonValue item;
                if (!ParseValue(item))
                {
                    return false;
                }

                value.arrayValue.push_back(std::move(item));
                SkipWhitespace();

                if (Consume(']'))
                {
                    return true;
                }

                if (!Consume(','))
                {
                    return false;
                }
            }

            return false;
        }

        bool ParseString(std::string& value)
        {
            if (!Consume('"'))
            {
                return false;
            }

            value.clear();

            while (m_Index < m_Text.size())
            {
                const char token = m_Text[m_Index++];
                if (token == '"')
                {
                    return true;
                }

                if (token != '\\')
                {
                    value.push_back(token);
                    continue;
                }

                if (m_Index >= m_Text.size())
                {
                    return false;
                }

                const char escape = m_Text[m_Index++];
                switch (escape)
                {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                case 'u':
                    if (m_Index + 4 > m_Text.size())
                    {
                        return false;
                    }
                    value.push_back('?');
                    m_Index += 4;
                    break;
                default:
                    return false;
                }
            }

            return false;
        }

        bool ParseNumber(double& value)
        {
            const std::size_t start = m_Index;

            if (m_Text[m_Index] == '-')
            {
                ++m_Index;
            }

            while (m_Index < m_Text.size() && std::isdigit(static_cast<unsigned char>(m_Text[m_Index])) != 0)
            {
                ++m_Index;
            }

            if (m_Index < m_Text.size() && m_Text[m_Index] == '.')
            {
                ++m_Index;
                while (m_Index < m_Text.size() && std::isdigit(static_cast<unsigned char>(m_Text[m_Index])) != 0)
                {
                    ++m_Index;
                }
            }

            if (m_Index < m_Text.size() && (m_Text[m_Index] == 'e' || m_Text[m_Index] == 'E'))
            {
                ++m_Index;
                if (m_Index < m_Text.size() && (m_Text[m_Index] == '+' || m_Text[m_Index] == '-'))
                {
                    ++m_Index;
                }

                while (m_Index < m_Text.size() && std::isdigit(static_cast<unsigned char>(m_Text[m_Index])) != 0)
                {
                    ++m_Index;
                }
            }

            try
            {
                value = std::stod(m_Text.substr(start, m_Index - start));
            }
            catch (...)
            {
                return false;
            }

            return true;
        }

        void SkipWhitespace()
        {
            while (m_Index < m_Text.size() && std::isspace(static_cast<unsigned char>(m_Text[m_Index])) != 0)
            {
                ++m_Index;
            }
        }

        bool Consume(char token)
        {
            SkipWhitespace();
            if (m_Index >= m_Text.size() || m_Text[m_Index] != token)
            {
                return false;
            }

            ++m_Index;
            return true;
        }

        bool MatchKeyword(const char* keyword)
        {
            const std::size_t length = std::char_traits<char>::length(keyword);
            if (m_Text.compare(m_Index, length, keyword) != 0)
            {
                return false;
            }

            m_Index += length;
            return true;
        }

        const std::string& m_Text;
        std::size_t m_Index = 0;
    };

    bool LoadFileText(const std::string& path, std::string& text)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            return false;
        }

        std::ostringstream buffer;
        buffer << stream.rdbuf();
        text = buffer.str();
        return true;
    }

    std::string EscapeJson(const std::string& text)
    {
        std::string escaped;
        escaped.reserve(text.size() + 8);

        for (char character : text)
        {
            switch (character)
            {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped.push_back(character); break;
            }
        }

        return escaped;
    }

    bool TryGetString(const JsonValue& value, std::string& result)
    {
        if (value.type != JsonValue::Type::String)
        {
            return false;
        }

        result = value.stringValue;
        return true;
    }

    bool TryGetNumber(const JsonValue& value, double& result)
    {
        if (value.type != JsonValue::Type::Number)
        {
            return false;
        }

        result = value.numberValue;
        return true;
    }

    bool TryGetInt(const JsonValue& value, int& result)
    {
        double number = 0.0;
        if (!TryGetNumber(value, number))
        {
            return false;
        }

        result = static_cast<int>(number);
        return true;
    }

    bool TryGetVec2Value(const JsonValue& value, honto::Vec2& result)
    {
        if (value.type == JsonValue::Type::Array && value.arrayValue.size() >= 2)
        {
            double x = 0.0;
            double y = 0.0;
            if (TryGetNumber(value.arrayValue[0], x) && TryGetNumber(value.arrayValue[1], y))
            {
                result = { static_cast<float>(x), static_cast<float>(y) };
                return true;
            }
        }

        if (value.type == JsonValue::Type::Object)
        {
            const JsonValue* xValue = value.Find("x");
            const JsonValue* yValue = value.Find("y");
            double x = 0.0;
            double y = 0.0;
            if (xValue != nullptr && yValue != nullptr && TryGetNumber(*xValue, x) && TryGetNumber(*yValue, y))
            {
                result = { static_cast<float>(x), static_cast<float>(y) };
                return true;
            }
        }

        return false;
    }

    bool TryGetColorValue(const JsonValue& value, honto::Color& result)
    {
        if (value.type == JsonValue::Type::Array && value.arrayValue.size() >= 3)
        {
            int r = 255;
            int g = 255;
            int b = 255;
            int a = 255;
            if (TryGetInt(value.arrayValue[0], r) && TryGetInt(value.arrayValue[1], g) && TryGetInt(value.arrayValue[2], b))
            {
                if (value.arrayValue.size() >= 4)
                {
                    TryGetInt(value.arrayValue[3], a);
                }

                result = {
                    static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
                    static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
                    static_cast<std::uint8_t>(std::clamp(b, 0, 255)),
                    static_cast<std::uint8_t>(std::clamp(a, 0, 255))
                };
                return true;
            }
        }

        if (value.type == JsonValue::Type::Object)
        {
            const JsonValue* rValue = value.Find("r");
            const JsonValue* gValue = value.Find("g");
            const JsonValue* bValue = value.Find("b");
            const JsonValue* aValue = value.Find("a");
            int r = 255;
            int g = 255;
            int b = 255;
            int a = 255;

            if (rValue != nullptr && gValue != nullptr && bValue != nullptr &&
                TryGetInt(*rValue, r) && TryGetInt(*gValue, g) && TryGetInt(*bValue, b))
            {
                if (aValue != nullptr)
                {
                    TryGetInt(*aValue, a);
                }

                result = {
                    static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
                    static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
                    static_cast<std::uint8_t>(std::clamp(b, 0, 255)),
                    static_cast<std::uint8_t>(std::clamp(a, 0, 255))
                };
                return true;
            }
        }

        return false;
    }

    std::string GetObjectString(const JsonValue& object, const std::string& key, const std::string& fallback = {})
    {
        const JsonValue* value = object.Find(key);
        if (value == nullptr)
        {
            return fallback;
        }

        std::string text;
        return TryGetString(*value, text) ? text : fallback;
    }

    int GetObjectInt(const JsonValue& object, const std::string& key, int fallback = 0)
    {
        const JsonValue* value = object.Find(key);
        if (value == nullptr)
        {
            return fallback;
        }

        int result = fallback;
        return TryGetInt(*value, result) ? result : fallback;
    }

    double GetObjectNumber(const JsonValue& object, const std::string& key, double fallback = 0.0)
    {
        const JsonValue* value = object.Find(key);
        if (value == nullptr)
        {
            return fallback;
        }

        double result = fallback;
        return TryGetNumber(*value, result) ? result : fallback;
    }

    bool TryGetObjectVec2(const JsonValue& object, const std::string& key, honto::Vec2& result)
    {
        const JsonValue* value = object.Find(key);
        return value != nullptr && TryGetVec2Value(*value, result);
    }

    bool TryGetObjectColor(const JsonValue& object, const std::string& key, honto::Color& result)
    {
        const JsonValue* value = object.Find(key);
        return value != nullptr && TryGetColorValue(*value, result);
    }

    bool TryGetPropertyString(const JsonValue& object, const std::string& key, std::string& result)
    {
        const JsonValue* properties = object.Find("properties");
        if (properties == nullptr)
        {
            return false;
        }

        if (properties->type == JsonValue::Type::Object)
        {
            const JsonValue* value = properties->Find(key);
            return value != nullptr && TryGetString(*value, result);
        }

        if (properties->type == JsonValue::Type::Array)
        {
            for (const JsonValue& property : properties->arrayValue)
            {
                if (property.type != JsonValue::Type::Object)
                {
                    continue;
                }

                if (GetObjectString(property, "name") != key)
                {
                    continue;
                }

                const JsonValue* value = property.Find("value");
                return value != nullptr && TryGetString(*value, result);
            }
        }

        return false;
    }

    bool TryGetPropertyInt(const JsonValue& object, const std::string& key, int& result)
    {
        const JsonValue* properties = object.Find("properties");
        if (properties == nullptr)
        {
            return false;
        }

        if (properties->type == JsonValue::Type::Object)
        {
            const JsonValue* value = properties->Find(key);
            return value != nullptr && TryGetInt(*value, result);
        }

        if (properties->type == JsonValue::Type::Array)
        {
            for (const JsonValue& property : properties->arrayValue)
            {
                if (property.type != JsonValue::Type::Object)
                {
                    continue;
                }

                if (GetObjectString(property, "name") != key)
                {
                    continue;
                }

                const JsonValue* value = property.Find("value");
                return value != nullptr && TryGetInt(*value, result);
            }
        }

        return false;
    }

    bool TryGetPropertyColor(const JsonValue& object, const std::string& key, honto::Color& result)
    {
        const JsonValue* properties = object.Find("properties");
        if (properties == nullptr)
        {
            return false;
        }

        if (properties->type == JsonValue::Type::Object)
        {
            const JsonValue* value = properties->Find(key);
            return value != nullptr && TryGetColorValue(*value, result);
        }

        if (properties->type == JsonValue::Type::Array)
        {
            for (const JsonValue& property : properties->arrayValue)
            {
                if (property.type != JsonValue::Type::Object)
                {
                    continue;
                }

                if (GetObjectString(property, "name") != key)
                {
                    continue;
                }

                const JsonValue* value = property.Find("value");
                return value != nullptr && TryGetColorValue(*value, result);
            }
        }

        return false;
    }

    char DefaultTileSymbol(int gid)
    {
        static const std::string symbols = ".#BCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmnopqrstuvwxyz";
        if (gid >= 0 && static_cast<std::size_t>(gid) < symbols.size())
        {
            return symbols[static_cast<std::size_t>(gid)];
        }

        return '#';
    }

    std::unordered_map<int, char> BuildTileSymbolMap(const JsonValue& root)
    {
        std::unordered_map<int, char> symbols;
        symbols[0] = '.';
        symbols[1] = '#';
        symbols[2] = 'B';
        symbols[3] = 'C';

        if (const JsonValue* tileSymbols = root.Find("tileSymbols"))
        {
            if (tileSymbols->type == JsonValue::Type::Array)
            {
                for (std::size_t index = 0; index < tileSymbols->arrayValue.size(); ++index)
                {
                    std::string symbol;
                    if (TryGetString(tileSymbols->arrayValue[index], symbol) && !symbol.empty())
                    {
                        symbols[static_cast<int>(index)] = symbol.front();
                    }
                }
            }
            else if (tileSymbols->type == JsonValue::Type::Object)
            {
                for (const auto& [key, value] : tileSymbols->objectValue)
                {
                    std::string symbol;
                    if (!TryGetString(value, symbol) || symbol.empty())
                    {
                        continue;
                    }

                    try
                    {
                        symbols[std::stoi(key)] = symbol.front();
                    }
                    catch (...)
                    {
                    }
                }
            }
        }

        if (const JsonValue* tilesets = root.Find("tilesets"); tilesets != nullptr && tilesets->type == JsonValue::Type::Array)
        {
            for (const JsonValue& tileset : tilesets->arrayValue)
            {
                if (tileset.type != JsonValue::Type::Object)
                {
                    continue;
                }

                const int firstGid = GetObjectInt(tileset, "firstgid", 1);
                const JsonValue* tiles = tileset.Find("tiles");
                if (tiles == nullptr || tiles->type != JsonValue::Type::Array)
                {
                    continue;
                }

                for (const JsonValue& tile : tiles->arrayValue)
                {
                    if (tile.type != JsonValue::Type::Object)
                    {
                        continue;
                    }

                    std::string symbol;
                    if (!TryGetPropertyString(tile, "symbol", symbol) || symbol.empty())
                    {
                        continue;
                    }

                    const int localId = GetObjectInt(tile, "id", 0);
                    symbols[firstGid + localId] = symbol.front();
                }
            }
        }

        return symbols;
    }

    bool ParseEntityJson(const JsonValue& value, honto::LevelEntity& entity)
    {
        if (value.type != JsonValue::Type::Object)
        {
            return false;
        }

        entity = {};
        entity.name = GetObjectString(value, "name");
        entity.kind = GetObjectString(value, "kind", GetObjectString(value, "type"));
        entity.layer = GetObjectInt(value, "layer", GetObjectInt(value, "z", 0));
        entity.text = GetObjectString(value, "text");
        entity.asset = GetObjectString(value, "asset");

        if (!TryGetObjectVec2(value, "position", entity.position))
        {
            entity.position = {
                static_cast<float>(GetObjectNumber(value, "x", 0.0)),
                static_cast<float>(GetObjectNumber(value, "y", 0.0))
            };
        }

        if (!TryGetObjectVec2(value, "size", entity.size))
        {
            entity.size = {
                static_cast<float>(GetObjectNumber(value, "width", 0.0)),
                static_cast<float>(GetObjectNumber(value, "height", 0.0))
            };
        }

        TryGetObjectColor(value, "color", entity.color);

        std::string propertyText;
        if (TryGetPropertyString(value, "text", propertyText))
        {
            entity.text = propertyText;
        }

        std::string propertyAsset;
        if (TryGetPropertyString(value, "asset", propertyAsset))
        {
            entity.asset = propertyAsset;
        }

        int propertyLayer = entity.layer;
        if (TryGetPropertyInt(value, "layer", propertyLayer))
        {
            entity.layer = propertyLayer;
        }

        honto::Color propertyColor = entity.color;
        if (TryGetPropertyColor(value, "color", propertyColor))
        {
            entity.color = propertyColor;
        }

        return true;
    }

    bool ParseHonToJson(const JsonValue& root, honto::LevelDocument& level)
    {
        if (root.type != JsonValue::Type::Object)
        {
            return false;
        }

        level = {};
        level.title = GetObjectString(root, "title", GetObjectString(root, "name"));

        if (!TryGetObjectVec2(root, "tileSize", level.tileSize))
        {
            if (!TryGetObjectVec2(root, "tile_size", level.tileSize))
            {
                level.tileSize = {
                    static_cast<float>(GetObjectNumber(root, "tilewidth", 16.0)),
                    static_cast<float>(GetObjectNumber(root, "tileheight", 16.0))
                };
            }
        }

        if (const JsonValue* map = root.Find("map"); map != nullptr && map->type == JsonValue::Type::Array)
        {
            for (const JsonValue& row : map->arrayValue)
            {
                std::string text;
                if (TryGetString(row, text))
                {
                    level.map.push_back(text);
                }
            }
        }

        if (const JsonValue* entities = root.Find("entities"); entities != nullptr && entities->type == JsonValue::Type::Array)
        {
            for (const JsonValue& entityValue : entities->arrayValue)
            {
                honto::LevelEntity entity;
                if (ParseEntityJson(entityValue, entity))
                {
                    level.entities.push_back(std::move(entity));
                }
            }
        }

        return level.IsValid();
    }

    bool ParseTiledJson(const JsonValue& root, honto::LevelDocument& level)
    {
        if (root.type != JsonValue::Type::Object)
        {
            return false;
        }

        level = {};
        level.title = GetObjectString(root, "name", GetObjectString(root, "title", "Untitled Map"));
        level.tileSize = {
            static_cast<float>(GetObjectNumber(root, "tilewidth", 16.0)),
            static_cast<float>(GetObjectNumber(root, "tileheight", 16.0))
        };

        const int width = GetObjectInt(root, "width", 0);
        const int height = GetObjectInt(root, "height", 0);
        const auto tileSymbols = BuildTileSymbolMap(root);

        const JsonValue* layers = root.Find("layers");
        if (layers == nullptr || layers->type != JsonValue::Type::Array)
        {
            return false;
        }

        for (const JsonValue& layer : layers->arrayValue)
        {
            if (layer.type != JsonValue::Type::Object)
            {
                continue;
            }

            const std::string type = GetObjectString(layer, "type");
            if (type == "tilelayer")
            {
                if (const JsonValue* rows = layer.Find("rows"); rows != nullptr && rows->type == JsonValue::Type::Array)
                {
                    for (const JsonValue& row : rows->arrayValue)
                    {
                        std::string text;
                        if (TryGetString(row, text))
                        {
                            level.map.push_back(text);
                        }
                    }

                    continue;
                }

                const JsonValue* data = layer.Find("data");
                if (data == nullptr || data->type != JsonValue::Type::Array)
                {
                    continue;
                }

                const int layerWidth = GetObjectInt(layer, "width", width);
                const int layerHeight = GetObjectInt(layer, "height", height);
                if (layerWidth <= 0 || layerHeight <= 0)
                {
                    continue;
                }

                level.map.assign(static_cast<std::size_t>(layerHeight), std::string(static_cast<std::size_t>(layerWidth), '.'));
                const std::size_t cellCount = std::min(data->arrayValue.size(), static_cast<std::size_t>(layerWidth * layerHeight));
                for (std::size_t index = 0; index < cellCount; ++index)
                {
                    int gid = 0;
                    if (!TryGetInt(data->arrayValue[index], gid))
                    {
                        continue;
                    }

                    const auto found = tileSymbols.find(gid);
                    const char symbol = found != tileSymbols.end() ? found->second : DefaultTileSymbol(gid);
                    const int x = static_cast<int>(index % static_cast<std::size_t>(layerWidth));
                    const int y = static_cast<int>(index / static_cast<std::size_t>(layerWidth));
                    if (y >= 0 && y < layerHeight && x >= 0 && x < layerWidth)
                    {
                        level.map[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = symbol;
                    }
                }
            }
            else if (type == "objectgroup")
            {
                const JsonValue* objects = layer.Find("objects");
                if (objects == nullptr || objects->type != JsonValue::Type::Array)
                {
                    continue;
                }

                for (const JsonValue& object : objects->arrayValue)
                {
                    honto::LevelEntity entity;
                    if (ParseEntityJson(object, entity))
                    {
                        level.entities.push_back(std::move(entity));
                    }
                }
            }
        }

        return level.IsValid();
    }

    bool ParseLevelJson(const JsonValue& root, honto::LevelDocument& level)
    {
        if (root.Find("layers") != nullptr || root.Find("tilewidth") != nullptr || root.Find("tileheight") != nullptr)
        {
            return ParseTiledJson(root, level);
        }

        return ParseHonToJson(root, level);
    }

    void WriteEntityText(std::ofstream& stream, const honto::LevelEntity& entity)
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

    void WriteEntityJson(std::ofstream& stream, const honto::LevelEntity& entity, const std::string& indent)
    {
        stream << indent << "{\n";
        stream << indent << "  \"name\": \"" << EscapeJson(entity.name) << "\",\n";
        stream << indent << "  \"kind\": \"" << EscapeJson(entity.kind) << "\",\n";
        stream << indent << "  \"position\": [" << entity.position.x << ", " << entity.position.y << "],\n";
        stream << indent << "  \"size\": [" << entity.size.x << ", " << entity.size.y << "],\n";
        stream << indent << "  \"layer\": " << entity.layer << ",\n";
        stream << indent << "  \"color\": ["
               << static_cast<int>(entity.color.r) << ", "
               << static_cast<int>(entity.color.g) << ", "
               << static_cast<int>(entity.color.b) << ", "
               << static_cast<int>(entity.color.a) << "],\n";
        stream << indent << "  \"text\": \"" << EscapeJson(entity.text) << "\",\n";
        stream << indent << "  \"asset\": \"" << EscapeJson(entity.asset) << "\"\n";
        stream << indent << "}";
    }
}

namespace honto
{
    bool LevelFile::Save(const std::string& path, const LevelDocument& level)
    {
        if (EndsWithIgnoreCase(path, ".json"))
        {
            return SaveJson(path, level);
        }

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
            WriteEntityText(stream, entity);
        }

        return static_cast<bool>(stream);
    }

    bool LevelFile::Load(const std::string& path, LevelDocument& level)
    {
        if (EndsWithIgnoreCase(path, ".json"))
        {
            return LoadJson(path, level);
        }

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
                    ParseVec2Text(value, currentEntity.position);
                }
                else if (key == "size")
                {
                    ParseVec2Text(value, currentEntity.size);
                }
                else if (key == "layer")
                {
                    currentEntity.layer = std::stoi(value);
                }
                else if (key == "color")
                {
                    ParseColorText(value, currentEntity.color);
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
                ParseVec2Text(value, level.tileSize);
            }
        }

        return level.IsValid();
    }

    bool LevelFile::SaveJson(const std::string& path, const LevelDocument& level)
    {
        std::ofstream stream(path, std::ios::binary);
        if (!stream)
        {
            return false;
        }

        stream << "{\n";
        stream << "  \"title\": \"" << EscapeJson(level.title) << "\",\n";
        stream << "  \"tileSize\": [" << level.tileSize.x << ", " << level.tileSize.y << "],\n";
        stream << "  \"map\": [\n";
        for (std::size_t index = 0; index < level.map.size(); ++index)
        {
            stream << "    \"" << EscapeJson(level.map[index]) << "\"";
            if (index + 1 < level.map.size())
            {
                stream << ',';
            }
            stream << '\n';
        }
        stream << "  ],\n";
        stream << "  \"entities\": [\n";
        for (std::size_t index = 0; index < level.entities.size(); ++index)
        {
            WriteEntityJson(stream, level.entities[index], "    ");
            if (index + 1 < level.entities.size())
            {
                stream << ',';
            }
            stream << '\n';
        }
        stream << "  ]\n";
        stream << "}\n";

        return static_cast<bool>(stream);
    }

    bool LevelFile::LoadJson(const std::string& path, LevelDocument& level)
    {
        std::string text;
        if (!LoadFileText(path, text))
        {
            return false;
        }

        JsonValue root;
        JsonParser parser(text);
        if (!parser.Parse(root))
        {
            return false;
        }

        return ParseLevelJson(root, level);
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
