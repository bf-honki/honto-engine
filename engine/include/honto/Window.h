#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "Math.h"

#include <cstdint>
#include <string>

namespace honto
{
    class Window
    {
    public:
        Window(
            const std::string& title,
            int width,
            int height,
            bool resizable = true,
            bool borderless = false,
            float opacity = 1.0f,
            bool alwaysOnTop = false
        );
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool ProcessEvents();
        static void PumpMessages();
        void Present(const std::uint32_t* pixels, int sourceWidth, int sourceHeight);
        void SetTitle(const std::string& title);
        bool GetMouseRenderPosition(int sourceWidth, int sourceHeight, Vec2& outPosition) const;
        void SetOpacity(float opacity);
        float GetOpacity() const;
        void SetBorderless(bool enabled);
        bool IsBorderless() const;
        void SetResizable(bool enabled);
        bool IsResizable() const;
        void SetAlwaysOnTop(bool enabled);
        bool IsAlwaysOnTop() const;
        void SetClientSize(int width, int height);
        void SetPosition(int x, int y);
        void Center();
        void Focus();

        bool IsOpen() const
        {
            return m_IsOpen;
        }

        int ClientWidth() const
        {
            return m_ClientWidth;
        }

        int ClientHeight() const
        {
            return m_ClientHeight;
        }

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        void RegisterWindowClass();
        void ApplyStyle();
        DWORD WindowStyle() const;
        DWORD WindowExStyle() const;
        RECT MakeWindowRect(int clientWidth, int clientHeight) const;

        HINSTANCE m_Instance = nullptr;
        HWND m_Handle = nullptr;
        std::string m_Title;
        int m_ClientWidth = 0;
        int m_ClientHeight = 0;
        bool m_IsOpen = true;
        bool m_Resizable = true;
        bool m_Borderless = false;
        bool m_AlwaysOnTop = false;
        float m_Opacity = 1.0f;
        BITMAPINFO m_BitmapInfo {};
    };
}
