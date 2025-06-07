#pragma once

namespace TY::detail
{
    namespace EngineKeyboard
    {
        void Update();

        [[nodiscard]] bool IsDown(uint8_t code);

        [[nodiscard]] bool IsPressed(uint8_t code);

        [[nodiscard]] bool IsUp(uint8_t code);
    }
}
