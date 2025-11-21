#include "pch.h"
#include "Alignment.h"

namespace TY
{
    Float2 AlignmentToPivot(Alignment9 alignment)
    {
        switch (alignment)
        {
        case Alignment9::TopLeft:
            return Float2{0.0f, 0.0f};
        case Alignment9::TopCenter:
            return Float2{0.5f, 0.0f};
        case Alignment9::TopRight:
            return Float2{1.0f, 0.0f};
        case Alignment9::MiddleLeft:
            return Float2{0.0f, 0.5f};
        case Alignment9::MiddleCenter:
            return Float2{0.5f, 0.5f};
        case Alignment9::MiddleRight:
            return Float2{1.0f, 0.5f};
        case Alignment9::BottomLeft:
            return Float2{0.0f, 1.0f};
        case Alignment9::BottomCenter:
            return Float2{0.5f, 1.0f};
        case Alignment9::BottomRight:
            return Float2{1.0f, 1.0f};
        default:
            return {};
        }
    }

    std::optional<Direction4> PointToDirection(const Float2& point)
    {
        if (point.isZero())
        {
            return std::nullopt;
        }

        if (Abs(point.x) > Abs(point.y))
        {
            return point.x > 0.0f ? Direction4::Right : Direction4::Left;
        }
        else
        {
            return point.y > 0.0f ? Direction4::Down : Direction4::Up;
        }
    }

    Point DirectionToPoint(Direction4 dir)
    {
        constexpr std::array points = {
            Point{1, 0}, // Right
            Point{0, -1}, // Up
            Point{-1, 0}, // Left
            Point{0, 1} // Down
        };
        return points[static_cast<int>(dir)];
    }

    bool IsDirectionHorizontal(Direction4 dir)
    {
        return dir == Direction4::Left || dir == Direction4::Right;
    }

    Direction4 ReverseDirection(Direction4 dir)
    {
        return static_cast<Direction4>((static_cast<int>(dir) + 2) % 4);
    }
}
