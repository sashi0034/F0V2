#pragma once
#include "Value2D.h"

namespace TY
{
    struct GamepadButtonState
    {
        bool up;
        bool pressed;
        bool down;
    };

    struct GamepadInputState
    {
        std::array<GamepadButtonState, 32> buttons;

        GamepadButtonState povUp{};

        GamepadButtonState povDown{};

        GamepadButtonState povLeft{};

        GamepadButtonState povRight{};

        Float2 axisL{};

        Float2 axisR{};
    };
}
