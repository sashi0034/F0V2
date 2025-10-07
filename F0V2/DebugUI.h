#pragma once
#include "TY/Immediate2D.h"

namespace DebugUI
{
    bool Button(const RectF& region, const std::u32string& text);

    bool DragButton(const RectF& region, const std::u32string& text);

    bool ItemButton(const RectF& region, const std::u32string& text, bool active);

    bool ListSlider(
        int& startIndex, int pageCapacity, int listCount, const RectF& sliderRegion, const RectF& scrollRegion);
}
