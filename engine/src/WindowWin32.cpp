#include "honto/Window.h"

#include <algorithm>
#include <stdexcept>

namespace
{
    constexpr const char* kWindowClassName = "HonToEngineWindowClass";
}

namespace honto
{
    Window::Window(const std::string& title, int width, int height, bool resizable, bool borderless, float opacity, bool alwaysOnTop)
        : m_Instance(GetModuleHandleA(nullptr)),
          m_Title(title),
          m_ClientWidth(width),
          m_ClientHeight(height),
          m_Resizable(resizable),
          m_Borderless(borderless),
          m_AlwaysOnTop(alwaysOnTop),
          m_Opacity(std::clamp(opacity, 0.1f, 1.0f))
    {
        RegisterWindowClass();

        const RECT rect = MakeWindowRect(width, height);
        const DWORD exStyle = WindowExStyle();

        m_Handle = CreateWindowExA(
            exStyle,
            kWindowClassName,
            m_Title.c_str(),
            WindowStyle(),
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

        ApplyStyle();
        ShowWindow(m_Handle, SW_SHOW);
        UpdateWindow(m_Handle);
        m_IsVisible = true;
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
        if (deviceContext == nullptr)
        {
            return;
        }

        HDC backBufferContext = CreateCompatibleDC(deviceContext);
        HBITMAP backBufferBitmap = CreateCompatibleBitmap(deviceContext, clientWidth, clientHeight);
        if (backBufferContext == nullptr || backBufferBitmap == nullptr)
        {
            if (backBufferBitmap != nullptr)
            {
                DeleteObject(backBufferBitmap);
            }
            if (backBufferContext != nullptr)
            {
                DeleteDC(backBufferContext);
            }
            ReleaseDC(m_Handle, deviceContext);
            return;
        }

        HGDIOBJ previousBitmap = SelectObject(backBufferContext, backBufferBitmap);
        FillRect(backBufferContext, &clientRect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        SetStretchBltMode(backBufferContext, COLORONCOLOR);

        StretchDIBits(
            backBufferContext,
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

        BitBlt(deviceContext, 0, 0, clientWidth, clientHeight, backBufferContext, 0, 0, SRCCOPY);

        SelectObject(backBufferContext, previousBitmap);
        DeleteObject(backBufferBitmap);
        DeleteDC(backBufferContext);
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

    void Window::SetOpacity(float opacity)
    {
        m_Opacity = std::clamp(opacity, 0.1f, 1.0f);
        ApplyStyle();
    }

    float Window::GetOpacity() const
    {
        return m_Opacity;
    }

    void Window::SetBorderless(bool enabled)
    {
        m_Borderless = enabled;
        ApplyStyle();
    }

    bool Window::IsBorderless() const
    {
        return m_Borderless;
    }

    void Window::SetResizable(bool enabled)
    {
        m_Resizable = enabled;
        ApplyStyle();
    }

    bool Window::IsResizable() const
    {
        return m_Resizable;
    }

    void Window::SetAlwaysOnTop(bool enabled)
    {
        m_AlwaysOnTop = enabled;
        ApplyStyle();
    }

    bool Window::IsAlwaysOnTop() const
    {
        return m_AlwaysOnTop;
    }

    void Window::SetClientSize(int width, int height)
    {
        m_ClientWidth = std::max(1, width);
        m_ClientHeight = std::max(1, height);

        if (m_Handle == nullptr)
        {
            return;
        }

        const RECT rect = MakeWindowRect(m_ClientWidth, m_ClientHeight);
        SetWindowPos(
            m_Handle,
            nullptr,
            0,
            0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
        );
    }

    void Window::SetPosition(int x, int y)
    {
        if (m_Handle == nullptr)
        {
            return;
        }

        SetWindowPos(
            m_Handle,
            nullptr,
            x,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
        );
    }

    void Window::Center()
    {
        if (m_Handle == nullptr)
        {
            return;
        }

        RECT windowRect {};
        GetWindowRect(m_Handle, &windowRect);

        HMONITOR monitor = MonitorFromWindow(m_Handle, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo {};
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfoA(monitor, &monitorInfo);

        const RECT work = monitorInfo.rcWork;
        const int width = windowRect.right - windowRect.left;
        const int height = windowRect.bottom - windowRect.top;
        const int x = work.left + ((work.right - work.left - width) / 2);
        const int y = work.top + ((work.bottom - work.top - height) / 2);
        SetPosition(x, y);
    }

    void Window::Hide()
    {
        if (m_Handle == nullptr)
        {
            m_IsVisible = false;
            return;
        }

        ShowWindow(m_Handle, SW_HIDE);
        m_IsVisible = false;
    }

    void Window::Focus()
    {
        if (m_Handle == nullptr)
        {
            return;
        }

        ShowWindow(m_Handle, SW_SHOW);
        m_IsVisible = true;
        SetForegroundWindow(m_Handle);
        SetFocus(m_Handle);
    }

    bool Window::HasFocus() const
    {
        return m_Handle != nullptr && GetForegroundWindow() == m_Handle;
    }

    void Window::Close()
    {
        if (m_Handle == nullptr)
        {
            m_IsOpen = false;
            m_IsVisible = false;
            return;
        }

        m_IsOpen = false;
        m_IsVisible = false;
        DestroyWindow(m_Handle);
        m_Handle = nullptr;
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
                window->m_IsVisible = false;
                window->m_Handle = nullptr;
                return 0;

            case WM_SIZE:
                window->m_ClientWidth = LOWORD(lParam);
                window->m_ClientHeight = HIWORD(lParam);
                return 0;

            case WM_SHOWWINDOW:
                window->m_IsVisible = wParam != FALSE;
                return 0;

            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT:
            {
                PAINTSTRUCT paint {};
                BeginPaint(hwnd, &paint);
                EndPaint(hwnd, &paint);
                return 0;
            }

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
        windowClass.style = CS_OWNDC;
        windowClass.lpfnWndProc = &Window::WindowProc;
        windowClass.hInstance = m_Instance;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = nullptr;
        windowClass.lpszClassName = kWindowClassName;

        if (RegisterClassExA(&windowClass) == 0)
        {
            throw std::runtime_error("Failed to register Win32 window class.");
        }

        s_IsRegistered = true;
    }

    void Window::ApplyStyle()
    {
        if (m_Handle == nullptr)
        {
            return;
        }

        SetWindowLongPtrA(m_Handle, GWL_STYLE, static_cast<LONG_PTR>(WindowStyle()));
        SetWindowLongPtrA(m_Handle, GWL_EXSTYLE, static_cast<LONG_PTR>(WindowExStyle()));

        if (m_Opacity < 0.999f)
        {
            SetLayeredWindowAttributes(
                m_Handle,
                0,
                static_cast<BYTE>(std::clamp(m_Opacity, 0.1f, 1.0f) * 255.0f),
                LWA_ALPHA
            );
        }

        const RECT rect = MakeWindowRect(m_ClientWidth, m_ClientHeight);
        SetWindowPos(
            m_Handle,
            m_AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
            0,
            0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW
        );
    }

    DWORD Window::WindowStyle() const
    {
        DWORD style = WS_VISIBLE;
        if (m_Borderless)
        {
            style |= WS_POPUP;
            return style;
        }

        style |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        if (m_Resizable)
        {
            style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        }

        return style;
    }

    DWORD Window::WindowExStyle() const
    {
        DWORD exStyle = 0;
        if (m_Opacity < 0.999f)
        {
            exStyle |= WS_EX_LAYERED;
        }

        return exStyle;
    }

    RECT Window::MakeWindowRect(int clientWidth, int clientHeight) const
    {
        RECT rect { 0, 0, clientWidth, clientHeight };
        AdjustWindowRectEx(&rect, WindowStyle(), FALSE, WindowExStyle());
        return rect;
    }
}
