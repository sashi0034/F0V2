#include "pch.h"
#include "Gamepad.h"

#include "detail/EngineGamepad.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    GamepadMapping s_mapping{}; // TODO: 複数のゲームパッド対応
}

namespace TY
{
    const GamepadInputState& GamepadInput::rawState() const
    {
        return EngineGamepad::GetInputState();
    }

    void GamepadInput::registerMapping(const GamepadMapping& mapping) const
    {
        s_mapping = mapping;
    }

    const GamepadButtonState& GamepadInput::dpadUp() const
    {
        return rawState().povUp;
    }

    const GamepadButtonState& GamepadInput::dpadDown() const
    {
        return rawState().povDown;
    }

    const GamepadButtonState& GamepadInput::dpadLeft() const
    {
        return rawState().povLeft;
    }

    const GamepadButtonState& GamepadInput::dpadRight() const
    {
        return rawState().povRight;
    }

    const GamepadButtonState& GamepadInput::a() const
    {
        return rawState().buttons[s_mapping.a];
    }

    const GamepadButtonState& GamepadInput::b() const
    {
        return rawState().buttons[s_mapping.b];
    }

    const GamepadButtonState& GamepadInput::x() const
    {
        return rawState().buttons[s_mapping.x];
    }

    const GamepadButtonState& GamepadInput::y() const
    {
        return rawState().buttons[s_mapping.y];
    }

    const GamepadButtonState& GamepadInput::lb() const
    {
        return rawState().buttons[s_mapping.lb];
    }

    const GamepadButtonState& GamepadInput::rb() const
    {
        return rawState().buttons[s_mapping.rb];
    }

    const GamepadButtonState& GamepadInput::lt() const
    {
        return rawState().buttons[s_mapping.lt];
    }

    const GamepadButtonState& GamepadInput::rt() const
    {
        return rawState().buttons[s_mapping.rt];
    }

    const GamepadButtonState& GamepadInput::menu() const
    {
        return rawState().buttons[s_mapping.menu];
    }

    const GamepadButtonState& GamepadInput::view() const
    {
        return rawState().buttons[s_mapping.view];
    }

    Float2 GamepadInput::axisL() const
    {
        return {rawState().axes[s_mapping.axis_lx], rawState().axes[s_mapping.axis_ly]};
    }

    Float2 GamepadInput::axisR() const
    {
        return {rawState().axes[s_mapping.axis_rx], rawState().axes[s_mapping.axis_ry]};
    }
}
