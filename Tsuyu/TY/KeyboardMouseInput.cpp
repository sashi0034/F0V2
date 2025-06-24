#include "pch.h"
#include "KeyboardMouseInput.h"

#include "detail/EngineKeyboardMouse.h"

namespace TY
{
    using namespace detail;

    bool KeyboardMouseInput::down() const
    {
        return EngineKeyboardMouse::KeyDown(m_code);
    }

    bool KeyboardMouseInput::pressed() const
    {
        return EngineKeyboardMouse::KeyPressed(m_code);
    }

    bool KeyboardMouseInput::up() const
    {
        return EngineKeyboardMouse::KeyUp(m_code);
    }
}
