#include "pch.h"
#include "EngineWindow.h"

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

struct EngineWindowImpl
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
    EngineWindowImpl s_engineWindow{};

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
    void EngineWindow::Init()
    {
        s_engineWindow.Init();
    }

    bool EngineWindow::IsInitialized()
    {
        return s_engineWindow.m_initialized;
    }

    void EngineWindow::Show()
    {
        s_engineWindow.Show();
    }

    void EngineWindow::Update()
    {
        s_engineWindow.Update();
    }

    void EngineWindow::AfterPresent()
    {
        s_engineWindow.AfterPresent();
    }

    HINSTANCE EngineWindow::HInstance()
    {
        return s_engineWindow.m_windowClass.hInstance;
    }

    HWND EngineWindow::Handle()
    {
        return s_engineWindow.m_handle;
    }

    Size EngineWindow::GetSize()
    {
        return s_engineWindow.m_windowSize;
    }

    int EngineWindow::TitleBarHeight()
    {
        return s_engineWindow.m_titleBarHeight;
    }

    float EngineWindow::WheelDelta()
    {
        return s_engineWindow.m_wheelDelta;
    }

    void EngineWindow::Resize(Size size)
    {
        s_engineWindow.Resize(size);
    }

    void EngineWindow::SetTitle(const std::wstring& title)
    {
        s_engineWindow.m_title = title;
        s_engineWindow.MakeFullTitle();
        SetWindowText(s_engineWindow.m_handle, s_engineWindow.m_fullTitle.c_str());
    }

    void EngineWindow::Shutdown()
    {
        s_engineWindow.Shutdown();
    }
}
