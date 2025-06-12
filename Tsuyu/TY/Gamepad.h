#pragma once
#include "GamepadInputState.h"

namespace TY
{
    class GamepadInput
    {
    public:
        const GamepadInputState& state() const;
    };

    inline constexpr GamepadInput MainGamepad{};
}
