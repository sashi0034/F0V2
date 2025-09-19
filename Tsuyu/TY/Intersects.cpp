#include "pch.h"
#include "Intersects.h"

namespace TY
{
    bool Intersects(const Float2& lhs, const RectF& rhs)
    {
        return not(lhs.x < rhs.leftX() || rhs.rightX() < lhs.x || lhs.y < rhs.topY() || rhs.bottomY() < lhs.y);
    }

    bool Intersects(const RectF& lhs, const Float2& rhs)
    {
        return Intersects(rhs, lhs);
    }

    bool Intersects(const RectF& lhs, const RectF& rhs)
    {
        return not(lhs.rightX() <= rhs.leftX() ||
            lhs.leftX() >= rhs.rightX() ||
            lhs.bottomY() <= rhs.topY() ||
            lhs.topY() >= rhs.bottomY());
    }
}
