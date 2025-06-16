#pragma once
#include "GamepadInputState.h"
#include "GamepadMapping.h"

namespace TY
{
    class GamepadInput
    {
    public:
        const GamepadInputState& rawState() const;

        void registerMapping(const GamepadMapping& mapping) const;

        const GamepadButtonState& dpadUp() const;
        const GamepadButtonState& dpadDown() const;
        const GamepadButtonState& dpadLeft() const;
        const GamepadButtonState& dpadRight() const;
        const GamepadButtonState& a() const;
        const GamepadButtonState& b() const;
        const GamepadButtonState& x() const;
        const GamepadButtonState& y() const;
        const GamepadButtonState& lb() const;
        const GamepadButtonState& rb() const;
        const GamepadButtonState& lt() const;
        const GamepadButtonState& rt() const;
        const GamepadButtonState& menu() const;
        const GamepadButtonState& view() const;

        Float2 axisL() const;
        Float2 axisR() const;

    private:
        int m_index{};
    };

    inline constexpr GamepadInput MainGamepad{};
}
