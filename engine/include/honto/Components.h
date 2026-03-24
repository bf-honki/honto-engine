#pragma once

#include "Color.h"
#include "Math.h"

namespace honto
{
    struct SpriteComponent
    {
        Vec2 size { 16.0f, 16.0f };
        Color color { 255, 255, 255, 255 };
        bool visible = true;
    };

    struct RigidBody2D
    {
        Vec2 velocity {};
    };
}
