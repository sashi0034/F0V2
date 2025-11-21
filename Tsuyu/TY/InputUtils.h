#pragma once
#include "Alignment.h"

namespace TY
{
    namespace KeyboardUtils
    {
        [[nodiscard]]
        std::optional<Direction4> GetTriggeredArrow();

        [[nodiscard]]
        std::optional<Direction4> GetTriggeredWASD();

        [[nodiscard]]
        std::optional<Direction4> GetTriggeredArrowOrWASD();
    }

    namespace GamepadUtils
    {
        [[nodiscard]]
        std::optional<Direction4> GetTriggeredDpad();

        [[nodiscard]]
        std::optional<Direction4> GetTriggeredLStick();

        [[nodiscard]]
        std::optional<Direction4> GetTriggeredDpadOrLStick();
    }
}
