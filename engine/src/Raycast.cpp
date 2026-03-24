#include "honto/Raycast.h"

#include "honto/Input.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kPi = 3.1415926535f;
}

namespace honto
{
    bool RaycastView::Init()
    {
        return true;
    }

    void RaycastView::SetMap(const std::vector<std::string>& map)
    {
        m_Map = map;
    }

    const std::vector<std::string>& RaycastView::GetMap() const
    {
        return m_Map;
    }

    void RaycastView::SetPlayer(const Vec2& position, float angleRadians)
    {
        m_PlayerPosition = position;
        m_PlayerAngle = angleRadians;
    }

    const Vec2& RaycastView::GetPlayerPosition() const
    {
        return m_PlayerPosition;
    }

    float RaycastView::GetPlayerAngle() const
    {
        return m_PlayerAngle;
    }

    void RaycastView::SetFieldOfView(float radians)
    {
        m_FieldOfView = std::clamp(radians, 0.3f, 2.6f);
    }

    float RaycastView::GetFieldOfView() const
    {
        return m_FieldOfView;
    }

    void RaycastView::SetMoveSpeed(float speed)
    {
        m_MoveSpeed = std::max(0.1f, speed);
    }

    void RaycastView::SetTurnSpeed(float speed)
    {
        m_TurnSpeed = std::max(0.1f, speed);
    }

    void RaycastView::EnableDoomControls(bool enabled)
    {
        m_ControlsEnabled = enabled;
        if (enabled)
        {
            ScheduleUpdate();
        }
    }

    void RaycastView::SetFloorColor(Color color)
    {
        m_FloorColor = color;
    }

    void RaycastView::SetCeilingColor(Color color)
    {
        m_CeilingColor = color;
    }

    void RaycastView::SetWallColor(char cell, Color color)
    {
        WallStyle& style = m_WallStyles[cell];
        style.color = color;
        style.hasColor = true;
    }

    void RaycastView::SetWallTexture(char cell, const std::shared_ptr<Texture>& texture)
    {
        m_WallStyles[cell].texture = texture;
    }

    void RaycastView::SetMiniMapEnabled(bool enabled, float scale)
    {
        m_MiniMapEnabled = enabled;
        m_MiniMapScale = std::max(2.0f, scale);
    }

    void RaycastView::SetMaxDistance(float distance)
    {
        m_MaxDistance = std::max(4.0f, distance);
    }

    void RaycastView::Update(float deltaTime)
    {
        if (!m_ControlsEnabled)
        {
            return;
        }

        float turn = 0.0f;
        if (Input::IsKeyDown(KeyCode::Left))
        {
            turn -= 1.0f;
        }

        if (Input::IsKeyDown(KeyCode::Right))
        {
            turn += 1.0f;
        }

        m_PlayerAngle += turn * m_TurnSpeed * deltaTime;
        while (m_PlayerAngle < -kPi)
        {
            m_PlayerAngle += kPi * 2.0f;
        }

        while (m_PlayerAngle > kPi)
        {
            m_PlayerAngle -= kPi * 2.0f;
        }

        const Vec2 forward { std::cos(m_PlayerAngle), std::sin(m_PlayerAngle) };
        const Vec2 right { -forward.y, forward.x };
        Vec2 movement {};

        if (Input::IsKeyDown(KeyCode::Up) || Input::IsKeyDown(KeyCode::W))
        {
            movement += forward;
        }

        if (Input::IsKeyDown(KeyCode::Down) || Input::IsKeyDown(KeyCode::S))
        {
            movement -= forward;
        }

        if (Input::IsKeyDown(KeyCode::A))
        {
            movement -= right;
        }

        if (Input::IsKeyDown(KeyCode::D))
        {
            movement += right;
        }

        if (movement.x != 0.0f || movement.y != 0.0f)
        {
            const float length = std::sqrt((movement.x * movement.x) + (movement.y * movement.y));
            movement *= (m_MoveSpeed * deltaTime) / std::max(length, 0.0001f);
            TryMove(movement);
        }
    }

    void RaycastView::Draw(Renderer2D& renderer, const Vec2& worldPosition, const Vec2& worldScale)
    {
        const Vec2 scaledSize = GetContentSize() * worldScale;
        const int viewportWidth = std::max(1, static_cast<int>(std::round(scaledSize.x)));
        const int viewportHeight = std::max(1, static_cast<int>(std::round(scaledSize.y)));

        renderer.DrawFilledRect(worldPosition, scaledSize, m_CeilingColor, false);
        renderer.DrawFilledRect(
            { worldPosition.x, worldPosition.y + (scaledSize.y * 0.5f) },
            { scaledSize.x, scaledSize.y * 0.5f },
            m_FloorColor,
            false
        );

        for (int column = 0; column < viewportWidth; ++column)
        {
            const float normalized = (static_cast<float>(column) / static_cast<float>(viewportWidth)) - 0.5f;
            const float rayAngle = m_PlayerAngle + (normalized * m_FieldOfView);
            RayHit hit = CastRay(rayAngle);
            if (!hit.hit || hit.distance <= 0.0001f)
            {
                continue;
            }

            const float correctedDistance = hit.distance * std::cos(rayAngle - m_PlayerAngle);
            const float safeDistance = std::max(0.0001f, correctedDistance);
            const float wallHeight = std::min(static_cast<float>(viewportHeight), static_cast<float>(viewportHeight) / safeDistance);
            const float wallTop = worldPosition.y + ((scaledSize.y - wallHeight) * 0.5f);
            const float wallBottom = wallTop + wallHeight;

            Color wallColor = ResolveWallColor(hit.cell);
            float shade = 1.0f - std::min(1.0f, hit.distance / m_MaxDistance);
            shade = std::max(0.25f, shade);
            if (hit.side)
            {
                shade *= 0.78f;
            }

            wallColor.r = static_cast<std::uint8_t>(static_cast<float>(wallColor.r) * shade);
            wallColor.g = static_cast<std::uint8_t>(static_cast<float>(wallColor.g) * shade);
            wallColor.b = static_cast<std::uint8_t>(static_cast<float>(wallColor.b) * shade);

            const int drawX = static_cast<int>(std::round(worldPosition.x)) + column;
            if (const auto* texture = ResolveWallTexture(hit.cell); texture != nullptr && *texture != nullptr && (*texture)->IsValid())
            {
                renderer.DrawTexturedColumn(drawX, wallTop, wallBottom, *(*texture), hit.textureU, wallColor, false);
            }
            else
            {
                renderer.DrawFilledRect({ static_cast<float>(drawX), wallTop }, { 1.0f, wallHeight }, wallColor, false);
            }
        }

        if (!m_MiniMapEnabled)
        {
            return;
        }

        const float tile = m_MiniMapScale;
        for (int y = 0; y < static_cast<int>(m_Map.size()); ++y)
        {
            for (int x = 0; x < static_cast<int>(m_Map[y].size()); ++x)
            {
                const char cell = m_Map[y][x];
                if (cell == '.' || cell == '0' || cell == ' ')
                {
                    renderer.DrawFilledRect(
                        { worldPosition.x + (x * tile), worldPosition.y + (y * tile) },
                        { tile - 1.0f, tile - 1.0f },
                        { 20, 24, 30, 220 },
                        false
                    );
                    continue;
                }

                renderer.DrawFilledRect(
                    { worldPosition.x + (x * tile), worldPosition.y + (y * tile) },
                    { tile - 1.0f, tile - 1.0f },
                    ResolveWallColor(cell),
                    false
                );
            }
        }

        renderer.DrawFilledRect(
            {
                worldPosition.x + (m_PlayerPosition.x * tile) - 1.5f,
                worldPosition.y + (m_PlayerPosition.y * tile) - 1.5f
            },
            { 3.0f, 3.0f },
            { 255, 255, 255, 255 },
            false
        );

        const Vec2 lookTarget {
            worldPosition.x + ((m_PlayerPosition.x + (std::cos(m_PlayerAngle) * 0.8f)) * tile),
            worldPosition.y + ((m_PlayerPosition.y + (std::sin(m_PlayerAngle) * 0.8f)) * tile)
        };
        renderer.DrawFilledRect(lookTarget, { 2.0f, 2.0f }, { 255, 220, 120, 255 }, false);
    }

    bool RaycastView::IsEmptyCell(int x, int y) const
    {
        if (y < 0 || y >= static_cast<int>(m_Map.size()))
        {
            return false;
        }

        if (x < 0 || x >= static_cast<int>(m_Map[y].size()))
        {
            return false;
        }

        const char cell = m_Map[y][x];
        return cell == '.' || cell == '0' || cell == ' ';
    }

    bool RaycastView::CanMoveTo(const Vec2& position) const
    {
        return IsEmptyCell(static_cast<int>(position.x), static_cast<int>(position.y));
    }

    void RaycastView::TryMove(const Vec2& delta)
    {
        Vec2 next = m_PlayerPosition;
        Vec2 test = next;
        test.x += delta.x;
        if (CanMoveTo(test))
        {
            next.x = test.x;
        }

        test = next;
        test.y += delta.y;
        if (CanMoveTo(test))
        {
            next.y = test.y;
        }

        m_PlayerPosition = next;
    }

    RaycastView::RayHit RaycastView::CastRay(float angle) const
    {
        RayHit result;
        const Vec2 rayDirection { std::cos(angle), std::sin(angle) };
        int mapX = static_cast<int>(m_PlayerPosition.x);
        int mapY = static_cast<int>(m_PlayerPosition.y);

        const float deltaDistanceX = (std::abs(rayDirection.x) < 0.0001f) ? 1.0e6f : std::abs(1.0f / rayDirection.x);
        const float deltaDistanceY = (std::abs(rayDirection.y) < 0.0001f) ? 1.0e6f : std::abs(1.0f / rayDirection.y);

        int stepX = 0;
        int stepY = 0;
        float sideDistanceX = 0.0f;
        float sideDistanceY = 0.0f;

        if (rayDirection.x < 0.0f)
        {
            stepX = -1;
            sideDistanceX = (m_PlayerPosition.x - static_cast<float>(mapX)) * deltaDistanceX;
        }
        else
        {
            stepX = 1;
            sideDistanceX = (static_cast<float>(mapX + 1) - m_PlayerPosition.x) * deltaDistanceX;
        }

        if (rayDirection.y < 0.0f)
        {
            stepY = -1;
            sideDistanceY = (m_PlayerPosition.y - static_cast<float>(mapY)) * deltaDistanceY;
        }
        else
        {
            stepY = 1;
            sideDistanceY = (static_cast<float>(mapY + 1) - m_PlayerPosition.y) * deltaDistanceY;
        }

        float distance = 0.0f;
        bool side = false;
        char hitCell = '#';

        while (distance < m_MaxDistance)
        {
            if (sideDistanceX < sideDistanceY)
            {
                sideDistanceX += deltaDistanceX;
                mapX += stepX;
                side = false;
            }
            else
            {
                sideDistanceY += deltaDistanceY;
                mapY += stepY;
                side = true;
            }

            if (!IsEmptyCell(mapX, mapY))
            {
                result.hit = true;
                hitCell = (mapY >= 0 && mapY < static_cast<int>(m_Map.size()) && mapX >= 0 && mapX < static_cast<int>(m_Map[mapY].size()))
                    ? m_Map[mapY][mapX]
                    : '#';

                if (!side)
                {
                    const float safeDirectionX = (std::abs(rayDirection.x) < 0.0001f) ? 0.0001f : rayDirection.x;
                    distance = (static_cast<float>(mapX) - m_PlayerPosition.x + ((1 - stepX) * 0.5f)) / safeDirectionX;
                }
                else
                {
                    const float safeDirectionY = (std::abs(rayDirection.y) < 0.0001f) ? 0.0001f : rayDirection.y;
                    distance = (static_cast<float>(mapY) - m_PlayerPosition.y + ((1 - stepY) * 0.5f)) / safeDirectionY;
                }

                break;
            }
        }

        if (!result.hit)
        {
            return result;
        }

        result.distance = std::max(0.001f, std::abs(distance));
        result.side = side;
        result.cell = hitCell;

        float wallCoordinate = 0.0f;
        if (!side)
        {
            wallCoordinate = m_PlayerPosition.y + (result.distance * rayDirection.y);
        }
        else
        {
            wallCoordinate = m_PlayerPosition.x + (result.distance * rayDirection.x);
        }

        wallCoordinate -= std::floor(wallCoordinate);
        result.textureU = wallCoordinate;
        return result;
    }

    Color RaycastView::ResolveWallColor(char cell) const
    {
        const auto found = m_WallStyles.find(cell);
        if (found != m_WallStyles.end() && found->second.hasColor)
        {
            return found->second.color;
        }

        const std::uint8_t seed = static_cast<std::uint8_t>(cell);
        return {
            static_cast<std::uint8_t>(80 + (seed * 31) % 120),
            static_cast<std::uint8_t>(70 + (seed * 17) % 120),
            static_cast<std::uint8_t>(60 + (seed * 23) % 120),
            255
        };
    }

    const std::shared_ptr<Texture>* RaycastView::ResolveWallTexture(char cell) const
    {
        const auto found = m_WallStyles.find(cell);
        if (found == m_WallStyles.end() || found->second.texture == nullptr)
        {
            return nullptr;
        }

        return &found->second.texture;
    }
}
