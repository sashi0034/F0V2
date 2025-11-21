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

    // -----------------------------------------------

    enum class Direction2 : uint8_t
    {
        Horizontal,
        Vertical,
    };

    // -----------------------------------------------

    enum class Direction4 : uint8_t
    {
        Right,
        Up,
        Left,
        Down,
    };

    [[nodiscard]]
    std::optional<Direction4> PointToDirection(const Float2& point);

    [[nodiscard]]
    Point DirectionToPoint(Direction4 dir);

    [[nodiscard]]
    bool IsDirectionHorizontal(Direction4 dir);

    [[nodiscard]]
    Direction4 ReverseDirection(Direction4 dir);
}
