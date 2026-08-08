#include "pch.h"
#include "Window.h"

#include "detail/Window_singleton.h"

using namespace TY;
using namespace TY::detail;

namespace TY
{
    void Window::SetTitle(const UnifiedString& title)
    {
        Window_singleton::SetTitle(title);
    }

    Size Window::GetSize()
    {
        return Window_singleton::GetSize();
    }

    void Window::Resize(Size size)
    {
        Window_singleton::Resize(size);
    }

    Point Window::GetPosition()
    {
        return Window_singleton::GetPosition();
    }

    void Window::SetPosition(Point position)
    {
        Window_singleton::SetPosition(position);
    }

    void Window::SetForeground()
    {
        SetForegroundWindow(Window_singleton::Handle());
    }

    bool Window::IsActive()
    {
        return Window_singleton::IsActive();
    }
}
