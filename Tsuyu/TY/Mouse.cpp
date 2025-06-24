#include "pch.h"
#include "Mouse.h"

#include "detail/EngineKeyboardMouse.h"

using namespace TY;
using namespace TY::detail;

namespace TY
{
    Point Mouse::Pos()
    {
        return EngineKeyboardMouse::MousePos().asPoint();
    }

    Float2 Mouse::PosF()
    {
        return EngineKeyboardMouse::MousePos();
    }
}
