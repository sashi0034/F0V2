#include "pch.h"
#include "Mouse.h"

#include "detail/EngineKeyboardMouse.h"

using namespace TY;
using namespace TY::detail;

namespace TY
{
    Float2 Mouse::PosF()
    {
        return EngineKeyboardMouse::MousePos();
    }
}
