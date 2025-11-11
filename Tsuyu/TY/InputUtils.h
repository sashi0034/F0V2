#pragma once
#include "Alignment.h"

namespace TY
{
    namespace KeyboardUtils
    {
        std::optional<Direction4> GetTriggeredArrow();

        std::optional<Direction4> GetTriggeredWASD();

        std::optional<Direction4> GetTriggeredArrowOrWASD();
    }

    namespace GamepadUtils
    {
        std::optional<Direction4> GetTriggeredDpad();
    }
}
