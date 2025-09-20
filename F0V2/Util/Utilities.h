#pragma once
#include "TY/Array.h"
#include "TY/Rect.h"

namespace Util
{
    Array<RectF> SliceRectByLength(const RectF& rect, float length, Direction2 dir);
}
