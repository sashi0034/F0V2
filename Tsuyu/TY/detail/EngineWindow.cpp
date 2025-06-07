#include "pch.h"
#include "EngineWindow.h"

#include "TY/Value2D.h"

#include "backends/imgui_impl_win32.h"
#include "TY/EngineTimer.h"
#include "TY/Math.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr Point DefaultWindowSize{1280, 720};

    const std::wstring windowTitle = L"F0"; // TODO

    LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if (msg == WM_DESTROY)
        {
            PostQuitMessage(0);
            return 0;
        }

        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) return true;

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    class EngineWindowImpl
    {
    public:
        void Init()
        {
            m_windowClass.cbSize = sizeof(WNDCLASSEX);
            m_windowClass.lpfnWndProc = static_cast<WNDPROC>(WindowProcedure);
            m_windowClass.lpszClassName = windowTitle.data();
            m_windowClass.hInstance = GetModuleHandle(nullptr);
            RegisterClassEx(&m_windowClass);

            // -----------------------------------------------

            m_windowSize = DefaultWindowSize;

            RECT windowRect{0, 0, m_windowSize.x, m_windowSize.y};
            AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

            m_handle = CreateWindow(
                m_windowClass.lpszClassName,
                L"F0",
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

        void Update()
        {
            const double dt = EngineTimer.GetDeltaTime();
            if (dt == 0.0) return;

            m_frameCount++;

            m_titleUpdateTimer -= dt;
            if (m_titleUpdateTimer <= 0.0)
            {
                m_titleUpdateTimer = 1.0;
                ApplyWindowTitle(m_frameCount);

                m_frameCount = 0;
            }
        }

        void ApplyWindowTitle(int fps)
        {
            wchar_t title[256];
            swprintf_s(title, L"%s | %d FPS", windowTitle.c_str(), fps);
            SetWindowText(m_handle, title);
        }

        void Shutdown()
        {
            UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
        }

        Point WindowSize() const { return m_windowSize; }

        HWND Handle() const { return m_handle; }

    private:
        WNDCLASSEX m_windowClass{};
        Point m_windowSize{};
        HWND m_handle{};

        int m_frameCount{};

        double m_titleUpdateTimer{1.0};
    } s_engineWindow{};
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
        return s_engineWindow.Handle();
    }

    Size EngineWindow::WindowSize()
    {
        return s_engineWindow.WindowSize();
    }

    void EngineWindow::Shutdown()
    {
        s_engineWindow.Shutdown();
    }
}
