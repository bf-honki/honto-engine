#pragma once

#include "SceneGraph.h"
#include "Texture.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace honto
{
    struct TileStyle
    {
        Color color { 255, 255, 255, 255 };
        std::shared_ptr<Texture> texture;
        TextureRegion region {};
        bool useRegion = false;
        bool solid = false;
        bool visible = true;
    };

    class TileMap : public Node
    {
    public:
        bool Init() override;
        HONTO_CREATE_FUNC(TileMap)

        static std::shared_ptr<TileMap> Create(const std::vector<std::string>& map, float tileWidth, float tileHeight);

        void SetMap(const std::vector<std::string>& map);
        const std::vector<std::string>& GetMap() const;

        void SetTileSize(const Vec2& size);
        void SetTileSize(float width, float height);
        const Vec2& GetTileSize() const;

        void SetTile(char tile, Color color, bool solid = false, bool visible = true);
        void SetTileTexture(char tile, const std::shared_ptr<Texture>& texture, Color tint = { 255, 255, 255, 255 }, bool solid = false, bool visible = true);
        void SetTileTextureRegion(char tile, const std::shared_ptr<Texture>& texture, const TextureRegion& region, Color tint = { 255, 255, 255, 255 }, bool solid = false, bool visible = true);
        void SetTileSolid(char tile, bool solid);
        void SetTileVisible(char tile, bool visible);

        bool IsSolidAtCell(int column, int row) const;
        bool IsSolidAtWorldPoint(const Vec2& point) const;
        bool CollidesRect(const Vec2& position, const Vec2& size) const;
        void ResolveMovement(Vec2& position, const Vec2& size, Vec2& velocity, const Vec2& delta, bool& collidedX, bool& collidedY, bool& onGround) const;

        void Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale) override;

    private:
        char TileAt(int column, int row) const;
        TileStyle GetStyle(char tile) const;
        bool IsEmptyTile(char tile) const;
        void UpdateContentSize();
        void ResolveHorizontal(Vec2& position, const Vec2& size, Vec2& velocity, float deltaX, bool& collided) const;
        void ResolveVertical(Vec2& position, const Vec2& size, Vec2& velocity, float deltaY, bool& collided, bool& onGround) const;

        std::vector<std::string> m_Map;
        Vec2 m_TileSize { 16.0f, 16.0f };
        std::unordered_map<char, TileStyle> m_TileStyles;
    };
}
