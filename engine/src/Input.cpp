#include "honto/Input.h"

namespace honto
{
    bool Input::s_Current[256] = {};
    bool Input::s_Previous[256] = {};

    void Input::Update()
    {
        for (int key = 0; key < 256; ++key)
        {
            s_Previous[key] = s_Current[key];
            s_Current[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
        }
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
}
