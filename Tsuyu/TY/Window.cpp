#include "pch.h"
#include "Window.h"

#include "detail/EngineWindow.h"

using namespace TY;
using namespace TY::detail;

namespace TY
{
    void Window::SetTitle(const UnifiedString& title)
    {
        EngineWindow::SetTitle(title);
    }
}
