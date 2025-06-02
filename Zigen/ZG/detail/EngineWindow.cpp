#include "pch.h"
#include "EngineWindow.h"

#include "ZG/Value2D.h"

namespace
{
    using namespace ZG;

    constexpr Point DefaultWindowSize{1280, 720};

    LRESULT windowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if (msg == WM_DESTROY)
        {
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    class Impl
    {
    public:
        void Init()
        {
            m_windowClass.cbSize = sizeof(WNDCLASSEX);
            m_windowClass.lpfnWndProc = static_cast<WNDPROC>(windowProcedure);
            m_windowClass.lpszClassName = L"F0";
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

        void Destroy()
        {
            UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
        }

        Point WindowSize() const { return m_windowSize; }

        HWND Handle() const { return m_handle; }

    private:
        WNDCLASSEX m_windowClass{};
        Point m_windowSize{};
        HWND m_handle{};
    } s_engineWindow;
}

namespace ZG::detail
{
    void EngineWindow_impl::Init() const
    {
        s_engineWindow.Init();
    }

    void EngineWindow_impl::Show() const
    {
        s_engineWindow.Show();
    }

    HWND EngineWindow_impl::Handle() const
    {
        return s_engineWindow.Handle();
    }

    Size EngineWindow_impl::WindowSize() const
    {
        return s_engineWindow.WindowSize();
    }

    void EngineWindow_impl::Destroy() const
    {
        s_engineWindow.Destroy();
    }
}
