#pragma once
#include "TY/GamepadInputState.h"

namespace TY::detail
{
    namespace Gamepad_singleton
    {
        void Init();

        void Update();

        void Shutdown();

        void OnDeviceChanged();

        const GamepadInputState& GetInputState();

        bool IsUsingGamepad();
    };
}
