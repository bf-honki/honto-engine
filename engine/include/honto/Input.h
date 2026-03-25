#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "Math.h"

#include <unordered_map>

namespace honto
{
    enum class KeyCode : int
    {
        Tab = VK_TAB,
        Left = VK_LEFT,
        Right = VK_RIGHT,
        Up = VK_UP,
        Down = VK_DOWN,
        Escape = VK_ESCAPE,
        Enter = VK_RETURN,
        Shift = VK_SHIFT,
        F1 = VK_F1,
        F2 = VK_F2,
        F3 = VK_F3,
        F4 = VK_F4,
        F5 = VK_F5,
        Space = VK_SPACE,
        A = 'A',
        D = 'D',
        E = 'E',
        M = 'M',
        Q = 'Q',
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
        static void UpdateForWindow(const Window& window, int renderWidth, int renderHeight);
        static bool IsKeyDown(KeyCode key);
        static bool IsKeyPressed(KeyCode key);
        static bool IsMouseDown(MouseButton button);
        static bool IsMousePressed(MouseButton button);
        static Vec2 MousePosition();
        static bool HasMouse();

    private:
        struct ContextState
        {
            bool current[256] {};
            bool previous[256] {};
            bool currentMouse[3] {};
            bool previousMouse[3] {};
            Vec2 mousePosition {};
            bool hasMouse = false;
        };

        static ContextState* ActiveState();
        static std::unordered_map<const Window*, ContextState> s_ContextStates;
        static const Window* s_ActiveWindow;
    };
}
