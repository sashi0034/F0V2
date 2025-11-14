#pragma once
#include "TY/GamepadInputState.h"

namespace TY::detail
{
    namespace Gamepad_singleton
    {
        void Init();

        void Update();

        void Shutdown();

        const GamepadInputState& GetInputState();

        bool IsUsingGamepad();
    };
}
