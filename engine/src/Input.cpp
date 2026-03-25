#include "honto/Input.h"

#include "honto/Window.h"

namespace honto
{
    std::unordered_map<const Window*, Input::ContextState> Input::s_ContextStates {};
    const Window* Input::s_ActiveWindow = nullptr;

    Input::ContextState* Input::ActiveState()
    {
        if (s_ActiveWindow == nullptr)
        {
            return nullptr;
        }

        const auto found = s_ContextStates.find(s_ActiveWindow);
        if (found == s_ContextStates.end())
        {
            return nullptr;
        }

        return &found->second;
    }

    void Input::Update()
    {
    }

    void Input::SetMouseContext(const Window& window, int renderWidth, int renderHeight)
    {
        UpdateForWindow(window, renderWidth, renderHeight);
    }

    void Input::UpdateForWindow(const Window& window, int renderWidth, int renderHeight)
    {
        s_ActiveWindow = &window;
        ContextState& state = s_ContextStates[s_ActiveWindow];

        const bool focused = window.IsVisible() && window.HasFocus();

        for (int key = 0; key < 256; ++key)
        {
            state.previous[key] = state.current[key];
            state.current[key] = focused && ((GetAsyncKeyState(key) & 0x8000) != 0);
        }

        const int mouseKeys[3] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
        for (int index = 0; index < 3; ++index)
        {
            state.previousMouse[index] = state.currentMouse[index];
            state.currentMouse[index] = focused && ((GetAsyncKeyState(mouseKeys[index]) & 0x8000) != 0);
        }

        state.mousePosition = {};
        state.hasMouse = focused && window.GetMouseRenderPosition(renderWidth, renderHeight, state.mousePosition);
    }

    bool Input::IsKeyDown(KeyCode key)
    {
        if (const ContextState* state = ActiveState())
        {
            return state->current[static_cast<int>(key)];
        }

        return false;
    }

    bool Input::IsKeyPressed(KeyCode key)
    {
        if (const ContextState* state = ActiveState())
        {
            const int index = static_cast<int>(key);
            return state->current[index] && !state->previous[index];
        }

        return false;
    }

    bool Input::IsMouseDown(MouseButton button)
    {
        if (const ContextState* state = ActiveState())
        {
            return state->currentMouse[static_cast<int>(button)];
        }

        return false;
    }

    bool Input::IsMousePressed(MouseButton button)
    {
        if (const ContextState* state = ActiveState())
        {
            const int index = static_cast<int>(button);
            return state->currentMouse[index] && !state->previousMouse[index];
        }

        return false;
    }

    Vec2 Input::MousePosition()
    {
        if (const ContextState* state = ActiveState())
        {
            return state->mousePosition;
        }

        return {};
    }

    bool Input::HasMouse()
    {
        if (const ContextState* state = ActiveState())
        {
            return state->hasMouse;
        }

        return false;
    }
}
