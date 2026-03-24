#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

namespace honto
{
    enum class KeyCode : int
    {
        Left = VK_LEFT,
        Right = VK_RIGHT,
        Up = VK_UP,
        Down = VK_DOWN,
        Escape = VK_ESCAPE,
        Enter = VK_RETURN,
        Space = VK_SPACE,
        A = 'A',
        D = 'D',
        S = 'S',
        W = 'W'
    };

    class Input
    {
    public:
        static void Update();
        static bool IsKeyDown(KeyCode key);
        static bool IsKeyPressed(KeyCode key);

    private:
        static bool s_Current[256];
        static bool s_Previous[256];
    };
}
