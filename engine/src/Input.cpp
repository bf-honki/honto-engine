#include "honto/Input.h"

#include "honto/Window.h"

namespace honto
{
    bool Input::s_Current[256] = {};
    bool Input::s_Previous[256] = {};
    bool Input::s_CurrentMouse[3] = {};
    bool Input::s_PreviousMouse[3] = {};
    Vec2 Input::s_MousePosition = {};
    bool Input::s_HasMouse = false;

    void Input::Update()
    {
        for (int key = 0; key < 256; ++key)
        {
            s_Previous[key] = s_Current[key];
            s_Current[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
        }

        const int mouseKeys[3] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
        for (int index = 0; index < 3; ++index)
        {
            s_PreviousMouse[index] = s_CurrentMouse[index];
            s_CurrentMouse[index] = (GetAsyncKeyState(mouseKeys[index]) & 0x8000) != 0;
        }
    }

    void Input::SetMouseContext(const Window& window, int renderWidth, int renderHeight)
    {
        s_HasMouse = window.GetMouseRenderPosition(renderWidth, renderHeight, s_MousePosition);
    }

    bool Input::IsKeyDown(KeyCode key)
    {
        return s_Current[static_cast<int>(key)];
    }

    bool Input::IsKeyPressed(KeyCode key)
    {
        const int index = static_cast<int>(key);
        return s_Current[index] && !s_Previous[index];
    }

    bool Input::IsMouseDown(MouseButton button)
    {
        return s_CurrentMouse[static_cast<int>(button)];
    }

    bool Input::IsMousePressed(MouseButton button)
    {
        const int index = static_cast<int>(button);
        return s_CurrentMouse[index] && !s_PreviousMouse[index];
    }

    Vec2 Input::MousePosition()
    {
        return s_MousePosition;
    }

    bool Input::HasMouse()
    {
        return s_HasMouse;
    }
}
