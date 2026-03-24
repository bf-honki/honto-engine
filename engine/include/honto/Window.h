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
        Window(const std::string& title, int width, int height);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool ProcessEvents();
        static void PumpMessages();
        void Present(const std::uint32_t* pixels, int sourceWidth, int sourceHeight);
        void SetTitle(const std::string& title);
        bool GetMouseRenderPosition(int sourceWidth, int sourceHeight, Vec2& outPosition) const;

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

        HINSTANCE m_Instance = nullptr;
        HWND m_Handle = nullptr;
        std::string m_Title;
        int m_ClientWidth = 0;
        int m_ClientHeight = 0;
        bool m_IsOpen = true;
        BITMAPINFO m_BitmapInfo {};
    };
}
