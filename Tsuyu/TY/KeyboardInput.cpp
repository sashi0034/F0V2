#include "pch.h"
#include "KeyboardInput.h"

#include "detail/EngineKeyboardMouse.h"

namespace TY
{
    using namespace detail;

    bool KeyboardInput::down() const
    {
        return EngineKeyboardMouse::KeyDown(m_code);
    }

    bool KeyboardInput::pressed() const
    {
        return EngineKeyboardMouse::KeyPressed(m_code);
    }

    bool KeyboardInput::up() const
    {
        return EngineKeyboardMouse::KeyUp(m_code);
    }
}
