#pragma once
#include "Array.h"
#include "Color.h"
#include "PrimitiveTypes3D.h"
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

            Line& setColor(const ColorF32& c0, const ColorF32& c1);

            void pushAuto();
        };

        struct LineSet
        {
            Array<std::pair<Float3, Float3>> lines;
            std::array<ColorF32, 2> colors = {ColorF32{1.0}, ColorF32{1.0}};

            LineSet& setColor(const ColorF32& color);

            LineSet& appendLine(const Float3& start, const Float3& end);

            LineSet& appendTriangle(const Triangle3D& tri);

            LineSet& appendAabb(const Aabb3D& aabb);

            void pushAuto();
        };

        // -----------------------------------------------

        using shape_type = Variant<Line, LineSet>;
    };
}
