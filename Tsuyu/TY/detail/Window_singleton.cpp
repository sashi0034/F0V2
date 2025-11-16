#include "pch.h"
#include "Window_singleton.h"

#include "RenderContext_singleton.h"
#include "TY/Vector2D.h"

#include "backends/imgui_impl_win32.h"
#include "EngineTimer.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr Point defaultWindowSize{1600, 900};

    LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    std::wstring getFullTitle(const std::wstring& title, int fps)
    {
        return std::format(L"{} | {} FPS", title, fps);
    }

    std::wstring getExecutableFileName()
    {
        wchar_t buffer[MAX_PATH];
        DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (length == 0 || length == MAX_PATH)
        {
            return L"Tsuyu_unknown";
        }

        std::filesystem::path path(buffer);
        return path.stem().wstring(); // 拡張子なしのファイル名
    }
}

struct WindowImpl
{
    bool m_initialized = false;

    WNDCLASSEX m_windowClass{};
    HWND m_handle{};

    Point m_windowSize{};
    int m_titleBarHeight{};
    float m_wheelDelta{};

    int m_frameCount{};
    int m_fps{};

    double m_titleUpdateTimer{1.0};

    std::wstring m_className{};
    std::wstring m_title{L"Tsuyu Application"};

    std::wstring m_fullTitle{m_title};

    bool m_fullscreen{false};

    std::optional<RECT> m_rectBeforeFullscreen{};

    void Init()
    {
        m_className = getExecutableFileName();

        m_windowClass.cbSize = sizeof(WNDCLASSEX);
        m_windowClass.lpfnWndProc = static_cast<WNDPROC>(WindowProcedure);
        m_windowClass.lpszClassName = m_className.c_str();
        m_windowClass.hInstance = GetModuleHandle(nullptr);
        RegisterClassEx(&m_windowClass);

        // -----------------------------------------------

        m_windowSize = defaultWindowSize;

        RECT windowRect{0, 0, m_windowSize.x, m_windowSize.y};
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

        RefreshTitleBar();

        MakeFullTitle();

        m_handle = CreateWindow(
            m_windowClass.lpszClassName,
            m_fullTitle.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr, // hWndParent
            nullptr, // hMenu 
            m_windowClass.hInstance, // hInstance, 
            nullptr // lpParam
        );

        m_initialized = true;
    }

    void Show()
    {
        ShowWindow(m_handle, SW_SHOW);
    }

    void MakeFullTitle()
    {
        m_fullTitle = getFullTitle(m_title, m_fps);
    }

    void Update()
    {
        const double dt = EngineTimer::GetDeltaTime();
        if (dt == 0.0)
        {
            return;
        }

        m_frameCount++;

        m_titleUpdateTimer -= dt;
        if (m_titleUpdateTimer <= 0.0)
        {
            m_titleUpdateTimer = 1.0;

            m_fps = m_frameCount;

            MakeFullTitle();

            SetWindowText(m_handle, m_fullTitle.c_str());

            m_frameCount = 0;
        }
    }

    void AfterPresent()
    {
        m_wheelDelta = 0.0f;
    }

    void Resize(const Point& newSize)
    {
        m_windowSize = newSize;

        RECT rect{0, 0, m_windowSize.x, m_windowSize.y};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        RefreshTitleBar();

        // FIXME: Remove it?
        SetWindowPos(
            m_handle,
            nullptr,
            0,
            0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER
        );
    }

    void SetBorderlessFullscreen(bool enable)
    {
        m_fullscreen = enable;

        RenderContext_singleton::RequestFullscreen(enable);

        const HWND hwnd = m_handle;

        if (enable)
        {
            if (RenderContext_singleton::IsFullscreen())
            {
                // DXGI 側で制御された場合は何もしない
                m_rectBeforeFullscreen = {};
                return;
            }

            {
                RECT rect;
                GetWindowRect(hwnd, &rect);
                m_rectBeforeFullscreen = rect;
            }

            MONITORINFO info = {sizeof(info)};
            GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &info);

            SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

            m_windowSize = {
                info.rcMonitor.right - info.rcMonitor.left,
                info.rcMonitor.bottom - info.rcMonitor.top
            };

            SetWindowPos(hwnd,
                         HWND_TOP,
                         info.rcMonitor.left,
                         info.rcMonitor.top,
                         m_windowSize.x,
                         m_windowSize.y,
                         SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
        }
        else // disable
        {
            if (not m_rectBeforeFullscreen.has_value())
            {
                // DXGI 側で制御された場合は何もしない
                return;
            }

            SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);

            m_windowSize = Size{
                m_rectBeforeFullscreen.value().right - m_rectBeforeFullscreen.value().left,
                m_rectBeforeFullscreen.value().bottom - m_rectBeforeFullscreen.value().top
            };

            SetWindowPos(hwnd,
                         HWND_NOTOPMOST,
                         m_rectBeforeFullscreen.value().left,
                         m_rectBeforeFullscreen.value().top,
                         m_windowSize.x,
                         m_windowSize.y,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            m_rectBeforeFullscreen = {};
        }
    }

    void RefreshTitleBar()
    {
        RECT windowRect{}, clientRect{};
        GetWindowRect(m_handle, &windowRect); // ウィンドウ全体
        GetClientRect(m_handle, &clientRect); // クライアント領域

        POINT clientTopLeft{0, 0};
        ClientToScreen(m_handle, &clientTopLeft);

        m_titleBarHeight = clientTopLeft.y - windowRect.top;
    }

    void Shutdown()
    {
        UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
    }
};

namespace
{
    WindowImpl s_engineWindow{};

    LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_SIZE: {
            s_engineWindow.m_windowSize = {LOWORD(lParam), HIWORD(lParam)};
            s_engineWindow.RefreshTitleBar();
            break;
        }
        case WM_MOUSEWHEEL: {
            s_engineWindow.m_wheelDelta += GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
            break;
        }
        case WM_SYSKEYDOWN:
            if (wParam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000))
            {
                s_engineWindow.SetBorderlessFullscreen(not s_engineWindow.m_fullscreen);
            }
        }

        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        {
            return true;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

namespace TY::detail
{
    void Window_singleton::Init()
    {
        s_engineWindow.Init();
    }

    bool Window_singleton::IsInitialized()
    {
        return s_engineWindow.m_initialized;
    }

    void Window_singleton::Show()
    {
        s_engineWindow.Show();
    }

    void Window_singleton::Update()
    {
        s_engineWindow.Update();
    }

    void Window_singleton::AfterPresent()
    {
        s_engineWindow.AfterPresent();
    }

    HINSTANCE Window_singleton::HInstance()
    {
        return s_engineWindow.m_windowClass.hInstance;
    }

    HWND Window_singleton::Handle()
    {
        return s_engineWindow.m_handle;
    }

    Size Window_singleton::GetSize()
    {
        return s_engineWindow.m_windowSize;
    }

    int Window_singleton::TitleBarHeight()
    {
        return s_engineWindow.m_titleBarHeight;
    }

    float Window_singleton::WheelDelta()
    {
        return s_engineWindow.m_wheelDelta;
    }

    bool Window_singleton::IsActive()
    {
        return GetActiveWindow() == s_engineWindow.m_handle;
    }

    void Window_singleton::Resize(Size size)
    {
        s_engineWindow.Resize(size);
    }

    void Window_singleton::SetTitle(const std::wstring& title)
    {
        s_engineWindow.m_title = title;
        s_engineWindow.MakeFullTitle();
        SetWindowText(s_engineWindow.m_handle, s_engineWindow.m_fullTitle.c_str());
    }

    void Window_singleton::Shutdown()
    {
        s_engineWindow.Shutdown();
    }
}
