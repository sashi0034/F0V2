#pragma once
#include "Vector2D.h"

namespace TY
{
    enum class Alignment9 : uint8_t
    {
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    Float2 AlignmentToPivot(Alignment9 alignment);
}
