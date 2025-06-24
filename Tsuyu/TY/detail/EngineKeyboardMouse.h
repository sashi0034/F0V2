#pragma once
#include "TY/Vector2D.h"

namespace TY::detail
{
    namespace EngineKeyboardMouse
    {
        void Update();

        [[nodiscard]] bool KeyDown(uint8_t code);

        [[nodiscard]] bool KeyPressed(uint8_t code);

        [[nodiscard]] bool KeyUp(uint8_t code);

        Float2 MousePos();

        Float2 PreviousMousePos();
    }
}
