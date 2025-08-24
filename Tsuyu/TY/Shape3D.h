#pragma once
#include "Color.h"
#include "Variant.h"
#include "Vector3D.h"

namespace TY
{
    namespace Shape3D
    {
        struct Line
        {
            Float3 start;
            Float3 end;
            std::array<ColorF32, 2> colors = {ColorF32{1.0}, ColorF32{1.0}};

            Line() = default;

            Line(const Float3& start_, const Float3& end_);

            Line& setColor(const ColorF32& color);
        };

        // -----------------------------------------------

        using shape_type = Variant<Line>;
    };
}
