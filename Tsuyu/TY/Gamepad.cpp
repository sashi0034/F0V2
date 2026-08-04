#include "pch.h"
#include "Gamepad.h"

#include "detail/Gamepad_singleton.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    GamepadMapping s_mapping{}; // TODO: 複数のゲームパッド対応

    float mappedTriggerValue(const GamepadInputState& state, bool previous = false)
    {
        const auto& axes = previous ? state.previousAxes : state.axes;
        float value = axes[s_mapping.axis_trigger];
        if (s_mapping.axis_trigger_inverted)
        {
            value = -value;
        }

        return value;
    }

    float mappedLeftTrigger(const GamepadInputState& state, bool previous = false)
    {
        return std::clamp(mappedTriggerValue(state, previous), 0.0f, 1.0f);
    }

    float mappedAxisRightTrigger(const GamepadInputState& state, bool previous = false)
    {
        return std::clamp(-mappedTriggerValue(state, previous), 0.0f, 1.0f);
    }

    const GamepadButtonState& triggerButtonState(bool left, const GamepadInputState& state)
    {
        static GamepadButtonState ltState{};
        static GamepadButtonState rtState{};

        GamepadButtonState& result = left ? ltState : rtState;
        const float current = left ? mappedLeftTrigger(state) : mappedAxisRightTrigger(state);
        const float previous = left ? mappedLeftTrigger(state, true) : mappedAxisRightTrigger(state, true);
        result.pressed = current >= TriggerButtonThreshold;
        result.down = result.pressed && previous < TriggerButtonThreshold;
        result.up = not result.pressed && previous >= TriggerButtonThreshold;
        return result;
    }
}

namespace TY
{
    const GamepadInputState& GamepadInput::rawState() const
    {
        return Gamepad_singleton::GetInputState();
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
        if (s_mapping.axis_trigger >= 0)
        {
            return triggerButtonState(true, rawState());
        }

        return rawState().buttons[s_mapping.lt];
    }

    const GamepadButtonState& GamepadInput::rt() const
    {
        if (s_mapping.axis_trigger >= 0)
        {
            return triggerButtonState(false, rawState());
        }

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

    float GamepadInput::leftTrigger() const
    {
        if (s_mapping.axis_trigger >= 0)
        {
            return mappedLeftTrigger(rawState());
        }
        return rawState().buttons[s_mapping.lt].pressed ? 1.0f : 0.0f;
    }

    float GamepadInput::rightTrigger() const
    {
        if (s_mapping.axis_trigger >= 0)
        {
            return mappedAxisRightTrigger(rawState());
        }
        return rawState().buttons[s_mapping.rt].pressed ? 1.0f : 0.0f;
    }

    Float2 GamepadInput::previousAxisL() const
    {
        return {rawState().previousAxes[s_mapping.axis_lx], rawState().previousAxes[s_mapping.axis_ly]};
    }

    Float2 GamepadInput::previousAxisR() const
    {
        return {rawState().previousAxes[s_mapping.axis_rx], rawState().previousAxes[s_mapping.axis_ry]};
    }

    bool IsUsingGamepad()
    {
        return Gamepad_singleton::IsUsingGamepad();
    }
}
