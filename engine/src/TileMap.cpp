#include "honto/TileMap.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kTileEpsilon = 0.001f;
}

namespace honto
{
    bool TileMap::Init()
    {
        return true;
    }

    std::shared_ptr<TileMap> TileMap::Create(const std::vector<std::string>& map, float tileWidth, float tileHeight)
    {
        auto tileMap = std::make_shared<TileMap>();
        if (tileMap != nullptr && tileMap->Init())
        {
            tileMap->SetTileSize(tileWidth, tileHeight);
            tileMap->SetMap(map);
            return tileMap;
        }

        return nullptr;
    }

    void TileMap::SetMap(const std::vector<std::string>& map)
    {
        m_Map = map;
        UpdateContentSize();
    }

    const std::vector<std::string>& TileMap::GetMap() const
    {
        return m_Map;
    }

    void TileMap::SetTileSize(const Vec2& size)
    {
        m_TileSize.x = std::max(1.0f, size.x);
        m_TileSize.y = std::max(1.0f, size.y);
        UpdateContentSize();
    }

    void TileMap::SetTileSize(float width, float height)
    {
        SetTileSize({ width, height });
    }

    const Vec2& TileMap::GetTileSize() const
    {
        return m_TileSize;
    }

    void TileMap::SetTile(char tile, Color color, bool solid, bool visible)
    {
        TileStyle style;
        style.color = color;
        style.solid = solid;
        style.visible = visible;
        m_TileStyles[tile] = style;
    }

    void TileMap::SetTileTexture(char tile, const std::shared_ptr<Texture>& texture, Color tint, bool solid, bool visible)
    {
        TileStyle style;
        style.color = tint;
        style.texture = texture;
        style.solid = solid;
        style.visible = visible;
        m_TileStyles[tile] = style;
    }

    void TileMap::SetTileTextureRegion(char tile, const std::shared_ptr<Texture>& texture, const TextureRegion& region, Color tint, bool solid, bool visible)
    {
        TileStyle style;
        style.color = tint;
        style.texture = texture;
        style.region = region;
        style.useRegion = region.IsValid();
        style.solid = solid;
        style.visible = visible;
        m_TileStyles[tile] = style;
    }

    void TileMap::SetTileSolid(char tile, bool solid)
    {
        TileStyle style = GetStyle(tile);
        style.solid = solid;
        m_TileStyles[tile] = style;
    }

    void TileMap::SetTileVisible(char tile, bool visible)
    {
        TileStyle style = GetStyle(tile);
        style.visible = visible;
        m_TileStyles[tile] = style;
    }

    bool TileMap::IsSolidAtCell(int column, int row) const
    {
        return GetStyle(TileAt(column, row)).solid;
    }

    bool TileMap::IsSolidAtWorldPoint(const Vec2& point) const
    {
        const Vec2 localPoint = point - GetPosition();
        const int column = static_cast<int>(std::floor(localPoint.x / m_TileSize.x));
        const int row = static_cast<int>(std::floor(localPoint.y / m_TileSize.y));
        return IsSolidAtCell(column, row);
    }

    bool TileMap::CollidesRect(const Vec2& position, const Vec2& size) const
    {
        if (m_Map.empty() || size.x <= 0.0f || size.y <= 0.0f)
        {
            return false;
        }

        const Vec2 localPosition = position - GetPosition();
        const int left = static_cast<int>(std::floor((localPosition.x + kTileEpsilon) / m_TileSize.x));
        const int right = static_cast<int>(std::floor((localPosition.x + size.x - kTileEpsilon) / m_TileSize.x));
        const int top = static_cast<int>(std::floor((localPosition.y + kTileEpsilon) / m_TileSize.y));
        const int bottom = static_cast<int>(std::floor((localPosition.y + size.y - kTileEpsilon) / m_TileSize.y));

        for (int row = top; row <= bottom; ++row)
        {
            for (int column = left; column <= right; ++column)
            {
                if (IsSolidAtCell(column, row))
                {
                    return true;
                }
            }
        }

        return false;
    }

    void TileMap::ResolveMovement(Vec2& position, const Vec2& size, Vec2& velocity, const Vec2& delta, bool& collidedX, bool& collidedY, bool& onGround) const
    {
        collidedX = false;
        collidedY = false;
        if (delta.y != 0.0f)
        {
            onGround = false;
        }

        if (m_Map.empty())
        {
            position += delta;
            return;
        }

        ResolveHorizontal(position, size, velocity, delta.x, collidedX);
        ResolveVertical(position, size, velocity, delta.y, collidedY, onGround);
    }

    void TileMap::Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale)
    {
        if (m_Map.empty())
        {
            return;
        }

        const Vec2 scaledTileSize = m_TileSize * worldScale;
        for (std::size_t row = 0; row < m_Map.size(); ++row)
        {
            const std::string& line = m_Map[row];
            for (std::size_t column = 0; column < line.size(); ++column)
            {
                const char tile = line[column];
                const TileStyle style = GetStyle(tile);
                if (!style.visible || IsEmptyTile(tile))
                {
                    continue;
                }

                const Vec2 tilePosition = {
                    worldPosition.x + (static_cast<float>(column) * scaledTileSize.x),
                    worldPosition.y + (static_cast<float>(row) * scaledTileSize.y)
                };

                if (style.texture != nullptr && style.texture->IsValid())
                {
                    if (style.useRegion)
                    {
                        renderer.DrawTexturedRectRegion(tilePosition, scaledTileSize, *style.texture, style.region, style.color);
                    }
                    else
                    {
                        renderer.DrawTexturedRect(tilePosition, scaledTileSize, *style.texture, style.color);
                    }
                }
                else
                {
                    renderer.DrawFilledRect(tilePosition, scaledTileSize, style.color);
                }
            }
        }
    }

    char TileMap::TileAt(int column, int row) const
    {
        if (row < 0 || row >= static_cast<int>(m_Map.size()))
        {
            return '\0';
        }

        const std::string& line = m_Map[static_cast<std::size_t>(row)];
        if (column < 0 || column >= static_cast<int>(line.size()))
        {
            return '\0';
        }

        return line[static_cast<std::size_t>(column)];
    }

    TileStyle TileMap::GetStyle(char tile) const
    {
        const auto found = m_TileStyles.find(tile);
        if (found != m_TileStyles.end())
        {
            return found->second;
        }

        TileStyle style;
        style.color = { 108, 118, 136, 255 };
        style.solid = !IsEmptyTile(tile);
        style.visible = !IsEmptyTile(tile);
        return style;
    }

    bool TileMap::IsEmptyTile(char tile) const
    {
        return tile == '\0' || tile == '.' || tile == ' ';
    }

    void TileMap::UpdateContentSize()
    {
        std::size_t maxColumns = 0;
        for (const std::string& line : m_Map)
        {
            maxColumns = std::max(maxColumns, line.size());
        }

        SetContentSize(
            static_cast<float>(maxColumns) * m_TileSize.x,
            static_cast<float>(m_Map.size()) * m_TileSize.y
        );
    }

    void TileMap::ResolveHorizontal(Vec2& position, const Vec2& size, Vec2& velocity, float deltaX, bool& collided) const
    {
        position.x += deltaX;
        if (deltaX == 0.0f || !CollidesRect(position, size))
        {
            return;
        }

        const Vec2 localPosition = position - GetPosition();
        const int top = static_cast<int>(std::floor((localPosition.y + kTileEpsilon) / m_TileSize.y));
        const int bottom = static_cast<int>(std::floor((localPosition.y + size.y - kTileEpsilon) / m_TileSize.y));

        if (deltaX > 0.0f)
        {
            const int right = static_cast<int>(std::floor((localPosition.x + size.x - kTileEpsilon) / m_TileSize.x));
            float resolvedX = position.x;

            for (int row = top; row <= bottom; ++row)
            {
                if (IsSolidAtCell(right, row))
                {
                    resolvedX = std::min(resolvedX, GetPosition().x + (static_cast<float>(right) * m_TileSize.x) - size.x);
                }
            }

            position.x = resolvedX;
        }
        else
        {
            const int left = static_cast<int>(std::floor((localPosition.x + kTileEpsilon) / m_TileSize.x));
            float resolvedX = position.x;

            for (int row = top; row <= bottom; ++row)
            {
                if (IsSolidAtCell(left, row))
                {
                    resolvedX = std::max(resolvedX, GetPosition().x + (static_cast<float>(left + 1) * m_TileSize.x));
                }
            }

            position.x = resolvedX;
        }

        velocity.x = 0.0f;
        collided = true;
    }

    void TileMap::ResolveVertical(Vec2& position, const Vec2& size, Vec2& velocity, float deltaY, bool& collided, bool& onGround) const
    {
        position.y += deltaY;
        if (deltaY == 0.0f || !CollidesRect(position, size))
        {
            return;
        }

        const Vec2 localPosition = position - GetPosition();
        const int left = static_cast<int>(std::floor((localPosition.x + kTileEpsilon) / m_TileSize.x));
        const int right = static_cast<int>(std::floor((localPosition.x + size.x - kTileEpsilon) / m_TileSize.x));

        if (deltaY > 0.0f)
        {
            const int bottom = static_cast<int>(std::floor((localPosition.y + size.y - kTileEpsilon) / m_TileSize.y));
            float resolvedY = position.y;

            for (int column = left; column <= right; ++column)
            {
                if (IsSolidAtCell(column, bottom))
                {
                    resolvedY = std::min(resolvedY, GetPosition().y + (static_cast<float>(bottom) * m_TileSize.y) - size.y);
                    onGround = true;
                }
            }

            position.y = resolvedY;
        }
        else
        {
            const int top = static_cast<int>(std::floor((localPosition.y + kTileEpsilon) / m_TileSize.y));
            float resolvedY = position.y;

            for (int column = left; column <= right; ++column)
            {
                if (IsSolidAtCell(column, top))
                {
                    resolvedY = std::max(resolvedY, GetPosition().y + (static_cast<float>(top + 1) * m_TileSize.y));
                }
            }

            position.y = resolvedY;
        }

        velocity.y = 0.0f;
        collided = true;
    }
}
