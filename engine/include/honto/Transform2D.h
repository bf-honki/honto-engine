#pragma once

#include "Math.h"

namespace honto
{
    struct Transform2D
    {
        Vec2 position {};
        Vec2 scale { 1.0f, 1.0f };
        float rotationDegrees = 0.0f;
    };
}
