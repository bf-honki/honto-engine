#pragma once

#include "Color.h"
#include "Math.h"
#include "SceneGraph.h"
#include "Texture.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace honto
{
    class RaycastView : public Node
    {
    public:
        bool Init() override;

        void SetMap(const std::vector<std::string>& map);
        const std::vector<std::string>& GetMap() const;

        void SetPlayer(const Vec2& position, float angleRadians);
        const Vec2& GetPlayerPosition() const;
        float GetPlayerAngle() const;

        void SetFieldOfView(float radians);
        float GetFieldOfView() const;

        void SetMoveSpeed(float speed);
        void SetTurnSpeed(float speed);
        void EnableDoomControls(bool enabled);

        void SetFloorColor(Color color);
        void SetCeilingColor(Color color);
        void SetWallColor(char cell, Color color);
        void SetWallTexture(char cell, const std::shared_ptr<Texture>& texture);

        void SetMiniMapEnabled(bool enabled, float scale = 8.0f);
        void SetMaxDistance(float distance);

        void Update(float deltaTime) override;
        void Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale) override;

    private:
        struct WallStyle
        {
            Color color { 255, 255, 255, 255 };
            std::shared_ptr<Texture> texture;
            bool hasColor = false;
        };

        struct RayHit
        {
            bool hit = false;
            float distance = 0.0f;
            float textureU = 0.0f;
            bool side = false;
            char cell = '#';
        };

        bool IsEmptyCell(int x, int y) const;
        bool CanMoveTo(const Vec2& position) const;
        void TryMove(const Vec2& delta);
        RayHit CastRay(float angle) const;
        Color ResolveWallColor(char cell) const;
        const std::shared_ptr<Texture>* ResolveWallTexture(char cell) const;

        std::vector<std::string> m_Map;
        std::unordered_map<char, WallStyle> m_WallStyles;
        Vec2 m_PlayerPosition { 2.5f, 2.5f };
        float m_PlayerAngle = 0.0f;
        float m_FieldOfView = 1.04719758f;
        float m_MoveSpeed = 3.0f;
        float m_TurnSpeed = 2.0f;
        float m_MaxDistance = 24.0f;
        Color m_FloorColor { 42, 42, 52, 255 };
        Color m_CeilingColor { 18, 20, 30, 255 };
        bool m_ControlsEnabled = false;
        bool m_MiniMapEnabled = true;
        float m_MiniMapScale = 8.0f;
    };
}
