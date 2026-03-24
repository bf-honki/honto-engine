#include "honto/Raycast.h"

#include "honto/Input.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    constexpr float kPi = 3.1415926535f;

    honto::Color Blend(honto::Color from, honto::Color to, float t)
    {
        const auto blend = [t](std::uint8_t a, std::uint8_t b)
        {
            return static_cast<std::uint8_t>(static_cast<float>(a) + ((static_cast<float>(b) - static_cast<float>(a)) * t));
        };

        return {
            blend(from.r, to.r),
            blend(from.g, to.g),
            blend(from.b, to.b),
            blend(from.a, to.a)
        };
    }
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
        m_DoorStates.clear();
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

    void RaycastView::SetRunMultiplier(float multiplier)
    {
        m_RunMultiplier = std::max(1.0f, multiplier);
    }

    void RaycastView::EnableDoomControls(bool enabled)
    {
        m_ControlsEnabled = enabled;
        if (enabled)
        {
            ScheduleUpdate();
        }
        else
        {
            UnscheduleUpdate();
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

    void RaycastView::SetDoor(char cell, Color color, float openSeconds, float holdSeconds)
    {
        DoorStyle& style = m_DoorStyles[cell];
        style.color = color;
        style.openSeconds = std::max(0.1f, openSeconds);
        style.holdSeconds = std::max(0.0f, holdSeconds);
    }

    void RaycastView::SetDoorTexture(char cell, const std::shared_ptr<Texture>& texture)
    {
        m_DoorStyles[cell].texture = texture;
    }

    void RaycastView::AddThing(std::string name, const Vec2& position, const Vec2& size, Color tint, const std::shared_ptr<Texture>& texture, float bobAmount, float bobSpeed)
    {
        Thing thing;
        thing.name = std::move(name);
        thing.position = position;
        thing.size = size;
        thing.tint = tint;
        thing.texture = texture;
        thing.bobAmount = bobAmount;
        thing.bobSpeed = bobSpeed;
        m_Things.push_back(std::move(thing));
    }

    void RaycastView::ClearThings()
    {
        m_Things.clear();
    }

    void RaycastView::SetWeapon(const std::shared_ptr<Texture>& texture, const Vec2& size, Color tint)
    {
        m_WeaponTexture = texture;
        m_WeaponSize = size;
        m_WeaponTint = tint;
    }

    void RaycastView::SetWeaponBobbing(float amount, float speed)
    {
        m_WeaponBobAmount = std::max(0.0f, amount);
        m_WeaponBobSpeed = std::max(0.0f, speed);
    }

    void RaycastView::SetFog(Color color, float strength)
    {
        m_FogColor = color;
        m_FogStrength = std::clamp(strength, 0.0f, 1.0f);
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
        UpdateDoors(deltaTime);

        if (!m_ControlsEnabled)
        {
            return;
        }

        if (Input::IsKeyPressed(KeyCode::Tab) || Input::IsKeyPressed(KeyCode::M))
        {
            m_MiniMapEnabled = !m_MiniMapEnabled;
        }

        if (Input::IsKeyPressed(KeyCode::E) || Input::IsKeyPressed(KeyCode::Enter))
        {
            TryUseDoor();
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

        const bool isRunning = Input::IsKeyDown(KeyCode::Shift);
        const float moveSpeed = m_MoveSpeed * (isRunning ? m_RunMultiplier : 1.0f);
        if (movement.x != 0.0f || movement.y != 0.0f)
        {
            const float length = std::sqrt((movement.x * movement.x) + (movement.y * movement.y));
            movement *= (moveSpeed * deltaTime) / std::max(length, 0.0001f);
            TryMove(movement);
            m_WeaponBobTime += deltaTime * (m_WeaponBobSpeed * (isRunning ? 1.25f : 1.0f));
        }
        else
        {
            m_WeaponBobTime += deltaTime * 2.0f;
        }

        if (Input::IsMousePressed(MouseButton::Left) || Input::IsKeyPressed(KeyCode::Space))
        {
            m_WeaponFlash = 0.08f;
        }

        m_WeaponFlash = std::max(0.0f, m_WeaponFlash - deltaTime);
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

        std::vector<float> zBuffer(static_cast<std::size_t>(viewportWidth), m_MaxDistance + 1.0f);

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
            zBuffer[static_cast<std::size_t>(column)] = safeDistance;

            const float wallHeight = std::min(static_cast<float>(viewportHeight), static_cast<float>(viewportHeight) / safeDistance);
            const float wallTop = worldPosition.y + ((scaledSize.y - wallHeight) * 0.5f);
            float wallBottom = wallTop + wallHeight;

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

            if (m_FogStrength > 0.0f)
            {
                const float fog = std::clamp((hit.distance / m_MaxDistance) * m_FogStrength, 0.0f, 1.0f);
                wallColor = Blend(wallColor, m_FogColor, fog);
            }

            if (hit.isDoor)
            {
                wallBottom = wallTop + (wallHeight * std::clamp(1.0f - hit.doorOpenAmount, 0.0f, 1.0f));
            }

            const float visibleHeight = wallBottom - wallTop;
            if (visibleHeight <= 0.5f)
            {
                continue;
            }

            const int drawX = static_cast<int>(std::round(worldPosition.x)) + column;
            if (const auto* texture = ResolveWallTexture(hit.cell); texture != nullptr && *texture != nullptr && (*texture)->IsValid())
            {
                renderer.DrawTexturedColumn(drawX, wallTop, wallBottom, *(*texture), hit.textureU, wallColor, false);
            }
            else
            {
                renderer.DrawFilledRect({ static_cast<float>(drawX), wallTop }, { 1.0f, visibleHeight }, wallColor, false);
            }
        }

        struct VisibleThing
        {
            const Thing* thing = nullptr;
            float distance = 0.0f;
        };

        std::vector<VisibleThing> visibleThings;
        visibleThings.reserve(m_Things.size());
        for (const Thing& thing : m_Things)
        {
            if (!thing.active)
            {
                continue;
            }

            const Vec2 delta = thing.position - m_PlayerPosition;
            visibleThings.push_back({ &thing, (delta.x * delta.x) + (delta.y * delta.y) });
        }

        std::sort(
            visibleThings.begin(),
            visibleThings.end(),
            [](const VisibleThing& left, const VisibleThing& right)
            {
                return left.distance > right.distance;
            }
        );

        const Vec2 direction { std::cos(m_PlayerAngle), std::sin(m_PlayerAngle) };
        const Vec2 plane { -std::sin(m_PlayerAngle) * std::tan(m_FieldOfView * 0.5f), std::cos(m_PlayerAngle) * std::tan(m_FieldOfView * 0.5f) };
        const float determinant = (plane.x * direction.y) - (direction.x * plane.y);
        const float inverseDeterminant = std::abs(determinant) < 0.0001f ? 0.0f : (1.0f / determinant);

        for (const VisibleThing& item : visibleThings)
        {
            const Thing& thing = *item.thing;
            Vec2 thingPosition = thing.position;
            if (thing.bobAmount > 0.0f && thing.bobSpeed > 0.0f)
            {
                thingPosition.y += std::sin((m_WeaponBobTime * thing.bobSpeed) + thing.position.x) * thing.bobAmount;
            }

            const Vec2 relative = thingPosition - m_PlayerPosition;
            const float transformX = inverseDeterminant * ((direction.y * relative.x) - (direction.x * relative.y));
            const float transformY = inverseDeterminant * ((-plane.y * relative.x) + (plane.x * relative.y));
            if (transformY <= 0.15f)
            {
                continue;
            }

            const float spriteScreenX = (static_cast<float>(viewportWidth) * 0.5f) * (1.0f + (transformX / transformY));
            const float spriteHeight = std::abs(static_cast<float>(viewportHeight) / transformY) * thing.size.y;
            const float spriteWidth = spriteHeight * thing.size.x;
            const float top = worldPosition.y + ((scaledSize.y - spriteHeight) * 0.5f);
            const float bottom = top + spriteHeight;
            const int startX = std::max(0, static_cast<int>(std::floor(spriteScreenX - (spriteWidth * 0.5f))));
            const int endX = std::min(viewportWidth - 1, static_cast<int>(std::ceil(spriteScreenX + (spriteWidth * 0.5f))));

            Color spriteTint = thing.tint;
            if (m_FogStrength > 0.0f)
            {
                const float fog = std::clamp((std::sqrt(item.distance) / m_MaxDistance) * m_FogStrength, 0.0f, 1.0f);
                spriteTint = Blend(spriteTint, m_FogColor, fog);
            }

            for (int stripe = startX; stripe <= endX; ++stripe)
            {
                if (stripe < 0 || stripe >= viewportWidth || transformY >= zBuffer[static_cast<std::size_t>(stripe)])
                {
                    continue;
                }

                const float u = std::clamp((static_cast<float>(stripe) - (spriteScreenX - (spriteWidth * 0.5f))) / std::max(1.0f, spriteWidth), 0.0f, 0.999f);
                const int drawX = static_cast<int>(std::round(worldPosition.x)) + stripe;

                if (thing.texture != nullptr && thing.texture->IsValid())
                {
                    renderer.DrawTexturedColumn(drawX, top, bottom, *thing.texture, u, spriteTint, false);
                }
                else
                {
                    renderer.DrawFilledRect({ static_cast<float>(drawX), top }, { 1.0f, spriteHeight }, spriteTint, false);
                }
            }
        }

        if (m_MiniMapEnabled)
        {
            const float tile = m_MiniMapScale;
            for (int y = 0; y < static_cast<int>(m_Map.size()); ++y)
            {
                for (int x = 0; x < static_cast<int>(m_Map[y].size()); ++x)
                {
                    const char cell = m_Map[y][x];
                    Color cellColor { 20, 24, 30, 220 };

                    if (!(cell == '.' || cell == '0' || cell == ' '))
                    {
                        cellColor = ResolveWallColor(cell);
                        if (IsDoorCell(x, y))
                        {
                            const float openAmount = DoorOpenAmount(x, y);
                            cellColor = Blend(cellColor, { 24, 28, 34, 220 }, openAmount);
                        }
                    }

                    renderer.DrawFilledRect(
                        { worldPosition.x + (x * tile), worldPosition.y + (y * tile) },
                        { tile - 1.0f, tile - 1.0f },
                        cellColor,
                        false
                    );
                }
            }

            for (const Thing& thing : m_Things)
            {
                if (!thing.active)
                {
                    continue;
                }

                renderer.DrawFilledRect(
                    { worldPosition.x + (thing.position.x * tile) - 1.0f, worldPosition.y + (thing.position.y * tile) - 1.0f },
                    { 3.0f, 3.0f },
                    thing.tint,
                    false
                );
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

        if (m_WeaponTexture != nullptr && m_WeaponTexture->IsValid())
        {
            const float bobX = std::sin(m_WeaponBobTime) * m_WeaponBobAmount;
            const float bobY = std::abs(std::cos(m_WeaponBobTime * 0.5f)) * m_WeaponBobAmount;
            const Vec2 weaponSize { m_WeaponSize.x * worldScale.x, m_WeaponSize.y * worldScale.y };
            const Vec2 weaponPosition {
                worldPosition.x + ((scaledSize.x - weaponSize.x) * 0.5f) + bobX,
                worldPosition.y + scaledSize.y - weaponSize.y + bobY
            };
            renderer.DrawTexturedRect(weaponPosition, weaponSize, *m_WeaponTexture, m_WeaponTint, false);

            if (m_WeaponFlash > 0.0f)
            {
                Color flash { 255, 228, 164, static_cast<std::uint8_t>(220.0f * (m_WeaponFlash / 0.08f)) };
                renderer.DrawFilledRect(
                    { worldPosition.x + (scaledSize.x * 0.5f) - 10.0f, worldPosition.y + (scaledSize.y * 0.62f) },
                    { 20.0f, 20.0f },
                    flash,
                    false
                );
            }
        }
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
        if (cell == '.' || cell == '0' || cell == ' ')
        {
            return true;
        }

        if (IsDoorCell(x, y))
        {
            return DoorOpenAmount(x, y) >= 0.98f;
        }

        return false;
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

    void RaycastView::UpdateDoors(float deltaTime)
    {
        std::vector<std::size_t> toRemove;
        toRemove.reserve(m_DoorStates.size());

        for (auto& [key, state] : m_DoorStates)
        {
            const int x = static_cast<int>(key & 0xFFFF);
            const int y = static_cast<int>((key >> 16) & 0xFFFF);
            if (y < 0 || y >= static_cast<int>(m_Map.size()) || x < 0 || x >= static_cast<int>(m_Map[y].size()))
            {
                toRemove.push_back(key);
                continue;
            }

            const char cell = m_Map[y][x];
            const auto styleFound = m_DoorStyles.find(cell);
            if (styleFound == m_DoorStyles.end())
            {
                toRemove.push_back(key);
                continue;
            }

            const DoorStyle& style = styleFound->second;
            if (state.opening)
            {
                state.openAmount += deltaTime / std::max(0.1f, style.openSeconds);
                if (state.openAmount >= 1.0f)
                {
                    state.openAmount = 1.0f;
                    state.opening = false;
                    state.holdTimer = style.holdSeconds;
                }
            }
            else if (state.openAmount > 0.0f)
            {
                if (state.holdTimer > 0.0f)
                {
                    state.holdTimer -= deltaTime;
                }
                else
                {
                    state.openAmount -= deltaTime / std::max(0.1f, style.openSeconds);
                    if (state.openAmount <= 0.0f)
                    {
                        toRemove.push_back(key);
                    }
                }
            }
        }

        for (std::size_t key : toRemove)
        {
            m_DoorStates.erase(key);
        }
    }

    void RaycastView::TryUseDoor()
    {
        const Vec2 forward { std::cos(m_PlayerAngle), std::sin(m_PlayerAngle) };
        const float sampleDistances[3] = { 0.8f, 1.05f, 1.25f };

        for (float distance : sampleDistances)
        {
            const Vec2 target = m_PlayerPosition + (forward * distance);
            const int cellX = static_cast<int>(target.x);
            const int cellY = static_cast<int>(target.y);
            if (!IsDoorCell(cellX, cellY))
            {
                continue;
            }

            DoorState& state = m_DoorStates[DoorKey(cellX, cellY)];
            state.opening = true;
            state.holdTimer = 0.0f;
            return;
        }
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
        bool hitDoor = false;
        float hitDoorOpenAmount = 0.0f;

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

            if (mapY < 0 || mapY >= static_cast<int>(m_Map.size()) || mapX < 0 || mapX >= static_cast<int>(m_Map[mapY].size()))
            {
                break;
            }

            if (IsEmptyCell(mapX, mapY))
            {
                continue;
            }

            result.hit = true;
            hitCell = m_Map[mapY][mapX];
            hitDoor = IsDoorCell(mapX, mapY);
            hitDoorOpenAmount = hitDoor ? DoorOpenAmount(mapX, mapY) : 0.0f;

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

        if (!result.hit)
        {
            return result;
        }

        result.distance = std::max(0.001f, std::abs(distance));
        result.side = side;
        result.cell = hitCell;
        result.isDoor = hitDoor;
        result.doorOpenAmount = hitDoorOpenAmount;

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
        if (const auto foundDoor = m_DoorStyles.find(cell); foundDoor != m_DoorStyles.end())
        {
            return foundDoor->second.color;
        }

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
        if (const auto foundDoor = m_DoorStyles.find(cell); foundDoor != m_DoorStyles.end() && foundDoor->second.texture != nullptr)
        {
            return &foundDoor->second.texture;
        }

        const auto found = m_WallStyles.find(cell);
        if (found == m_WallStyles.end() || found->second.texture == nullptr)
        {
            return nullptr;
        }

        return &found->second.texture;
    }

    bool RaycastView::IsDoorCell(int x, int y) const
    {
        if (y < 0 || y >= static_cast<int>(m_Map.size()) || x < 0 || x >= static_cast<int>(m_Map[y].size()))
        {
            return false;
        }

        return m_DoorStyles.find(m_Map[y][x]) != m_DoorStyles.end();
    }

    bool RaycastView::IsThingVisible(const Vec2& position) const
    {
        const Vec2 toThing = position - m_PlayerPosition;
        const float distanceSquared = (toThing.x * toThing.x) + (toThing.y * toThing.y);
        return distanceSquared > 0.01f && distanceSquared <= (m_MaxDistance * m_MaxDistance);
    }

    float RaycastView::DoorOpenAmount(int x, int y) const
    {
        const auto found = m_DoorStates.find(DoorKey(x, y));
        if (found == m_DoorStates.end())
        {
            return 0.0f;
        }

        return std::clamp(found->second.openAmount, 0.0f, 1.0f);
    }

    std::size_t RaycastView::DoorKey(int x, int y) const
    {
        return static_cast<std::size_t>((y << 16) | (x & 0xFFFF));
    }
}
