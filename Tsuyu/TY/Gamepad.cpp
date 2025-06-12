#include "pch.h"
#include "Gamepad.h"

#include "detail/EngineGamepad.h"

using namespace TY;
using namespace TY::detail;

namespace TY
{
    const GamepadInputState& GamepadInput::state() const
    {
        return EngineGamepad::GetInputState();
    }
}
