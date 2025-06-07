#include "pch.h"
#include "KeyboardInput.h"

#include "detail/EngineKeyboard.h"

namespace TY
{
    using namespace detail;

    bool KeyboardInput::down() const
    {
        return EngineKeyboard::IsDown(m_code);
    }

    bool KeyboardInput::pressed() const
    {
        return EngineKeyboard::IsPressed(m_code);
    }

    bool KeyboardInput::up() const
    {
        return EngineKeyboard::IsUp(m_code);
    }
}
