#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "Math.h"

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
        F5 = VK_F5,
        Space = VK_SPACE,
        A = 'A',
        D = 'D',
        S = 'S',
        W = 'W'
    };

    enum class MouseButton : int
    {
        Left = 0,
        Right = 1,
        Middle = 2
    };

    class Window;

    class Input
    {
    public:
        static void Update();
        static void SetMouseContext(const Window& window, int renderWidth, int renderHeight);
        static bool IsKeyDown(KeyCode key);
        static bool IsKeyPressed(KeyCode key);
        static bool IsMouseDown(MouseButton button);
        static bool IsMousePressed(MouseButton button);
        static Vec2 MousePosition();
        static bool HasMouse();

    private:
        static bool s_Current[256];
        static bool s_Previous[256];
        static bool s_CurrentMouse[3];
        static bool s_PreviousMouse[3];
        static Vec2 s_MousePosition;
        static bool s_HasMouse;
    };
}
