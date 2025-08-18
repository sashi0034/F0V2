#pragma once
#include "Array.h"
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

        struct SquareDotLine;

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

            SquareDotLine asDotLine(float dotOffset = 0.0f) const;
        };

        struct SquareDotLine
        {
            Line line;
            float dotOffset;

            SquareDotLine& setDotOffset(float offset_);
        };

        struct Path
        {
            Array<Float2> points;
            float thickness{1.0f};
            ColorF32 color{ColorF32{1.0}};

            Path() = default;

            Path(const Array<Float2>& points_);

            Path& append(const Float2& p);

            Path& setThickness(float thickness_);

            Path& setColor(const ColorF32& color_);
        };

        // -----------------------------------------------

        using shape_type = Variant<
            Rectangle,
            Line,
            SquareDotLine,
            Path
        >;
    }
}
