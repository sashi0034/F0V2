#pragma once
#include "Color.h"

namespace TY
{
    struct HSV
    {
        using value_type = double;

        value_type h;

        value_type s;

        value_type v;

        value_type alpha;

        HSV() = default;

        HSV(value_type h, value_type s, value_type v, value_type a = 1.0) :
            h(h),
            s(s),
            v(v),
            alpha(a)
        {
        }

        HSV(const ColorU8& color);

        HSV(const ColorF32& color);

        ColorF32 toColorF() const;
    };
}
