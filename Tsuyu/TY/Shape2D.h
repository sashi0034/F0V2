#pragma once
#include "Color.h"
#include "Rect.h"
#include "Variant.h"

namespace TY
{
    namespace Shape2D
    {
        struct Rectangle
        {
            RectF rect;
            std::array<ColorF32, 4> colors = {ColorF32{1.0}, ColorF32{1.0}, ColorF32{1.0}, ColorF32{1.0}};

            Rectangle() = default;

            Rectangle(const RectF& rect_);

            Rectangle& setColor(const ColorF32& color_);
        };

        struct Line
        {
            Float2 start;
            Float2 end;
            float thickness{1.0f};
            std::array<ColorF32, 2> colors = {ColorF32{1.0}, ColorF32{1.0}};

            Line() = default;

            Line(const Float2& start_, const Float2& end_);

            Line(float x1, float y1, float x2, float y2);

            Line& setThickness(float thickness_);

            Line& setColor(const ColorF32& color_);
        };

        // -----------------------------------------------

        using shape_type = Variant<Rectangle, Line>;
    }
}
