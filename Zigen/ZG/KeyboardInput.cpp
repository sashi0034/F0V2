#include "pch.h"
#include "KeyboardInput.h"

#include "detail/EngineKeyboard.h"

namespace ZG
{
    using namespace detail;

    bool KeyboardInput::down() const
    {
        return EngineKeyboard.isDown(m_code);
    }

    bool KeyboardInput::pressed() const
    {
        return EngineKeyboard.isPressed(m_code);
    }

    bool KeyboardInput::up() const
    {
        return EngineKeyboard.isUp(m_code);
    }
}
