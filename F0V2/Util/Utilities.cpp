#include "pch.h"
#include "Utilities.h"

Array<RectF> Util::SliceRectByLength(const RectF& rect, float length, Direction2 dir)
{
    const auto& pos = rect.pos;
    const auto& size = rect.size;
    const float edgeLength = dir == Direction2::Horizontal ? size.x : size.y;
    const float padding = std::fmod(edgeLength, length) / 2;
    const int count = static_cast<int>(edgeLength / length);

    Array<RectF> result(count);
    for (int i = 0; i < count; ++i)
    {
        if (dir == Direction2::Horizontal)
        {
            result[i] = RectF{Float2{pos.x + padding + length * i, pos.y}, Float2{length, size.y}};
        }
        else
        {
            result[i] = RectF{Float2{pos.x, pos.y + padding + length * i}, Float2{size.x, length}};
        }
    }

    return result;
}
