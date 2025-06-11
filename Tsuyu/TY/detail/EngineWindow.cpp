#include "pch.h"
#include "EngineWindow.h"

#include "TY/Value2D.h"

#include "backends/imgui_impl_win32.h"
#include "EngineTimer.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr Point defaultWindowSize{1280, 720};

    LRESULT windowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if (msg == WM_DESTROY)
        {
            PostQuitMessage(0);
            return 0;
        }

        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) return true;

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

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
    WNDCLASSEX m_windowClass{};
    Point m_windowSize{};
    HWND m_handle{};

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
        m_windowClass.lpfnWndProc = static_cast<WNDPROC>(windowProcedure);
        m_windowClass.lpszClassName = m_className.c_str();
        m_windowClass.hInstance = GetModuleHandle(nullptr);
        RegisterClassEx(&m_windowClass);

        // -----------------------------------------------

        m_windowSize = defaultWindowSize;

        RECT windowRect{0, 0, m_windowSize.x, m_windowSize.y};
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

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
        if (dt == 0.0) return;

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

    void Shutdown()
    {
        UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
    }
};

namespace
{
    EngineWindowImpl s_engineWindow{};
}

namespace TY::detail
{
    void EngineWindow::Init()
    {
        s_engineWindow.Init();
    }

    void EngineWindow::Show()
    {
        s_engineWindow.Show();
    }

    void EngineWindow::Update()
    {
        s_engineWindow.Update();
    }

    HWND EngineWindow::Handle()
    {
        return s_engineWindow.m_handle;
    }

    Size EngineWindow::WindowSize()
    {
        return s_engineWindow.m_windowSize;
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
