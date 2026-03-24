#include "honto/Window.h"

#include <algorithm>
#include <stdexcept>

namespace
{
    constexpr const char* kWindowClassName = "HonToEngineWindowClass";
}

namespace honto
{
    Window::Window(const std::string& title, int width, int height)
        : m_Instance(GetModuleHandleA(nullptr)),
          m_Title(title),
          m_ClientWidth(width),
          m_ClientHeight(height)
    {
        RegisterWindowClass();

        RECT rect { 0, 0, width, height };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        m_Handle = CreateWindowExA(
            0,
            kWindowClassName,
            m_Title.c_str(),
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            m_Instance,
            this
        );

        if (m_Handle == nullptr)
        {
            throw std::runtime_error("Failed to create Win32 window.");
        }

        ShowWindow(m_Handle, SW_SHOW);
        UpdateWindow(m_Handle);
    }

    Window::~Window()
    {
        if (m_Handle != nullptr)
        {
            DestroyWindow(m_Handle);
            m_Handle = nullptr;
        }
    }

    bool Window::ProcessEvents()
    {
        PumpMessages();
        return m_IsOpen;
    }

    void Window::PumpMessages()
    {
        MSG message {};

        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }

    void Window::Present(const std::uint32_t* pixels, int sourceWidth, int sourceHeight)
    {
        if (m_Handle == nullptr || pixels == nullptr || sourceWidth <= 0 || sourceHeight <= 0)
        {
            return;
        }

        RECT clientRect {};
        GetClientRect(m_Handle, &clientRect);

        const int clientWidth = clientRect.right - clientRect.left;
        const int clientHeight = clientRect.bottom - clientRect.top;
        const float scaleX = static_cast<float>(clientWidth) / static_cast<float>(sourceWidth);
        const float scaleY = static_cast<float>(clientHeight) / static_cast<float>(sourceHeight);
        const float scale = std::max(0.0f, std::min(scaleX, scaleY));

        const int destinationWidth = std::max(1, static_cast<int>(sourceWidth * scale));
        const int destinationHeight = std::max(1, static_cast<int>(sourceHeight * scale));
        const int offsetX = (clientWidth - destinationWidth) / 2;
        const int offsetY = (clientHeight - destinationHeight) / 2;

        m_BitmapInfo = {};
        m_BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        m_BitmapInfo.bmiHeader.biWidth = sourceWidth;
        m_BitmapInfo.bmiHeader.biHeight = -sourceHeight;
        m_BitmapInfo.bmiHeader.biPlanes = 1;
        m_BitmapInfo.bmiHeader.biBitCount = 32;
        m_BitmapInfo.bmiHeader.biCompression = BI_RGB;

        HDC deviceContext = GetDC(m_Handle);
        FillRect(deviceContext, &clientRect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

        StretchDIBits(
            deviceContext,
            offsetX,
            offsetY,
            destinationWidth,
            destinationHeight,
            0,
            0,
            sourceWidth,
            sourceHeight,
            pixels,
            &m_BitmapInfo,
            DIB_RGB_COLORS,
            SRCCOPY
        );

        ReleaseDC(m_Handle, deviceContext);
    }

    void Window::SetTitle(const std::string& title)
    {
        m_Title = title;
        if (m_Handle != nullptr)
        {
            SetWindowTextA(m_Handle, m_Title.c_str());
        }
    }

    bool Window::GetMouseRenderPosition(int sourceWidth, int sourceHeight, Vec2& outPosition) const
    {
        outPosition = {};

        if (m_Handle == nullptr || sourceWidth <= 0 || sourceHeight <= 0)
        {
            return false;
        }

        POINT mousePoint {};
        if (!GetCursorPos(&mousePoint))
        {
            return false;
        }

        if (!ScreenToClient(m_Handle, &mousePoint))
        {
            return false;
        }

        RECT clientRect {};
        GetClientRect(m_Handle, &clientRect);

        const int clientWidth = clientRect.right - clientRect.left;
        const int clientHeight = clientRect.bottom - clientRect.top;
        if (clientWidth <= 0 || clientHeight <= 0)
        {
            return false;
        }

        const float scaleX = static_cast<float>(clientWidth) / static_cast<float>(sourceWidth);
        const float scaleY = static_cast<float>(clientHeight) / static_cast<float>(sourceHeight);
        const float scale = std::max(0.0f, std::min(scaleX, scaleY));
        if (scale <= 0.0f)
        {
            return false;
        }

        const int destinationWidth = std::max(1, static_cast<int>(sourceWidth * scale));
        const int destinationHeight = std::max(1, static_cast<int>(sourceHeight * scale));
        const int offsetX = (clientWidth - destinationWidth) / 2;
        const int offsetY = (clientHeight - destinationHeight) / 2;

        const bool inside =
            mousePoint.x >= offsetX &&
            mousePoint.y >= offsetY &&
            mousePoint.x < offsetX + destinationWidth &&
            mousePoint.y < offsetY + destinationHeight;

        outPosition.x = static_cast<float>(mousePoint.x - offsetX) / scale;
        outPosition.y = static_cast<float>(mousePoint.y - offsetY) / scale;
        return inside;
    }

    LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Window* window = reinterpret_cast<Window*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

        if (message == WM_NCCREATE)
        {
            auto* createStruct = reinterpret_cast<CREATESTRUCTA*>(lParam);
            window = static_cast<Window*>(createStruct->lpCreateParams);
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        }

        if (window != nullptr)
        {
            switch (message)
            {
            case WM_CLOSE:
                window->m_IsOpen = false;
                DestroyWindow(hwnd);
                return 0;

            case WM_DESTROY:
                window->m_IsOpen = false;
                window->m_Handle = nullptr;
                return 0;

            case WM_SIZE:
                window->m_ClientWidth = LOWORD(lParam);
                window->m_ClientHeight = HIWORD(lParam);
                return 0;

            default:
                break;
            }
        }

        return DefWindowProcA(hwnd, message, wParam, lParam);
    }

    void Window::RegisterWindowClass()
    {
        static bool s_IsRegistered = false;
        if (s_IsRegistered)
        {
            return;
        }

        WNDCLASSEXA windowClass {};
        windowClass.cbSize = sizeof(WNDCLASSEXA);
        windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        windowClass.lpfnWndProc = &Window::WindowProc;
        windowClass.hInstance = m_Instance;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        windowClass.lpszClassName = kWindowClassName;

        if (RegisterClassExA(&windowClass) == 0)
        {
            throw std::runtime_error("Failed to register Win32 window class.");
        }

        s_IsRegistered = true;
    }
}
