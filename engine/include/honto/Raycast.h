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
        void SetRunMultiplier(float multiplier);
        void EnableDoomControls(bool enabled);

        void SetFloorColor(Color color);
        void SetCeilingColor(Color color);
        void SetWallColor(char cell, Color color);
        void SetWallTexture(char cell, const std::shared_ptr<Texture>& texture);
        void SetDoor(char cell, Color color, float openSeconds = 0.8f, float holdSeconds = 1.6f);
        void SetDoorTexture(char cell, const std::shared_ptr<Texture>& texture);
        void AddThing(
            std::string name,
            const Vec2& position,
            const Vec2& size,
            Color tint = { 255, 255, 255, 255 },
            const std::shared_ptr<Texture>& texture = nullptr,
            float bobAmount = 0.0f,
            float bobSpeed = 0.0f
        );
        void ClearThings();
        void SetWeapon(const std::shared_ptr<Texture>& texture, const Vec2& size, Color tint = { 255, 255, 255, 255 });
        void SetWeaponBobbing(float amount, float speed);
        void SetFog(Color color, float strength);

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
            bool isDoor = false;
            float doorOpenAmount = 0.0f;
        };

        bool IsEmptyCell(int x, int y) const;
        bool CanMoveTo(const Vec2& position) const;
        void TryMove(const Vec2& delta);
        void UpdateDoors(float deltaTime);
        void TryUseDoor();
        RayHit CastRay(float angle) const;
        Color ResolveWallColor(char cell) const;
        const std::shared_ptr<Texture>* ResolveWallTexture(char cell) const;
        bool IsDoorCell(int x, int y) const;
        bool IsThingVisible(const Vec2& position) const;
        float DoorOpenAmount(int x, int y) const;
        std::size_t DoorKey(int x, int y) const;

        std::vector<std::string> m_Map;
        std::unordered_map<char, WallStyle> m_WallStyles;
        struct DoorStyle
        {
            Color color { 174, 140, 96, 255 };
            std::shared_ptr<Texture> texture;
            float openSeconds = 0.8f;
            float holdSeconds = 1.6f;
        };

        struct DoorState
        {
            float openAmount = 0.0f;
            float holdTimer = 0.0f;
            bool opening = false;
        };

        struct Thing
        {
            std::string name;
            Vec2 position {};
            Vec2 size { 0.8f, 0.8f };
            Color tint { 255, 255, 255, 255 };
            std::shared_ptr<Texture> texture;
            float bobAmount = 0.0f;
            float bobSpeed = 0.0f;
            bool active = true;
        };

        std::unordered_map<char, DoorStyle> m_DoorStyles;
        std::unordered_map<std::size_t, DoorState> m_DoorStates;
        std::vector<Thing> m_Things;
        Vec2 m_PlayerPosition { 2.5f, 2.5f };
        float m_PlayerAngle = 0.0f;
        float m_FieldOfView = 1.04719758f;
        float m_MoveSpeed = 3.0f;
        float m_TurnSpeed = 2.0f;
        float m_RunMultiplier = 1.55f;
        float m_MaxDistance = 24.0f;
        Color m_FloorColor { 42, 42, 52, 255 };
        Color m_CeilingColor { 18, 20, 30, 255 };
        Color m_FogColor { 18, 22, 34, 255 };
        float m_FogStrength = 0.0f;
        std::shared_ptr<Texture> m_WeaponTexture;
        Vec2 m_WeaponSize { 128.0f, 72.0f };
        Color m_WeaponTint { 255, 255, 255, 255 };
        float m_WeaponBobAmount = 3.0f;
        float m_WeaponBobSpeed = 9.0f;
        float m_WeaponBobTime = 0.0f;
        float m_WeaponFlash = 0.0f;
        bool m_ControlsEnabled = false;
        bool m_MiniMapEnabled = true;
        float m_MiniMapScale = 8.0f;
    };
}
