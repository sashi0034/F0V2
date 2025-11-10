#pragma once
#include "Array.h"
#include "Vector2D.h"

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
        using axes_type = std::array<float, 6>; // lx, ly, lz, lRx, lRy, lRz

        std::array<GamepadButtonState, 32> buttons;

        GamepadButtonState povUp{};

        GamepadButtonState povDown{};

        GamepadButtonState povLeft{};

        GamepadButtonState povRight{};

        axes_type axes;

        Array<int> getDownButtonIndexes() const;

        Array<int> getActiveAxisIndexes(float threshold = 0.5f) const;
    };
}
