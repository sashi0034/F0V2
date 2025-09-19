#include "pch.h"
#include "Mouse.h"

#include "detail/EngineKeyboardMouse.h"
#include "detail/EngineWindow.h"

using namespace TY;
using namespace TY::detail;

namespace TY
{
    Point Mouse::PreviousPos()
    {
        return EngineKeyboardMouse::PreviousMousePos().asPoint();
    }

    Float2 Mouse::PreviousPosF()
    {
        return EngineKeyboardMouse::PreviousMousePos();
    }

    Point Mouse::Pos()
    {
        return EngineKeyboardMouse::MousePos().asPoint();
    }

    Float2 Mouse::PosF()
    {
        return EngineKeyboardMouse::MousePos();
    }

    float Mouse::Wheel()
    {
        return EngineWindow::GetWheelDelta();
    }
}
