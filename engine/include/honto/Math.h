#pragma once

namespace honto
{
    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;

        constexpr Vec2() = default;
        constexpr Vec2(float xValue, float yValue) : x(xValue), y(yValue) {}

        constexpr Vec2 operator+(const Vec2& other) const
        {
            return { x + other.x, y + other.y };
        }

        constexpr Vec2 operator-(const Vec2& other) const
        {
            return { x - other.x, y - other.y };
        }

        constexpr Vec2 operator*(float scalar) const
        {
            return { x * scalar, y * scalar };
        }

        constexpr Vec2 operator*(const Vec2& other) const
        {
            return { x * other.x, y * other.y };
        }

        constexpr Vec2 operator/(float scalar) const
        {
            return { x / scalar, y / scalar };
        }

        Vec2& operator+=(const Vec2& other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        Vec2& operator-=(const Vec2& other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        Vec2& operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        Vec2& operator*=(const Vec2& other)
        {
            x *= other.x;
            y *= other.y;
            return *this;
        }

        Vec2& operator/=(float scalar)
        {
            x /= scalar;
            y /= scalar;
            return *this;
        }
    };

    constexpr Vec2 operator*(float scalar, const Vec2& value)
    {
        return value * scalar;
    }
}
