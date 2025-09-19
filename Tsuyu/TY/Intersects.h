#pragma once
#include "Rect.h"

namespace TY
{
    // Priority:
    // - Point
    // - Float2
    // - Rect
    // - RectF

    // -----------------------------------------------
    // Float2

    bool Intersects(const Float2& lhs, const RectF& rhs);

    // -----------------------------------------------
    // RectF

    bool Intersects(const RectF& lhs, const Float2& rhs);

    bool Intersects(const RectF& lhs, const RectF& rhs);
}
