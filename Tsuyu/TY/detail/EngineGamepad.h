#pragma once
#include "TY/GamepadInputState.h"

namespace TY::detail
{
    namespace EngineGamepad
    {
        void Init();

        void Update();

        void Shutdown();

        const GamepadInputState& GetInputState();
    };
}
