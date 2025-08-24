#pragma once
#include "Color.h"
#include "Shape3D.h"
#include "ShapeBufferCreator.h"
#include "Vector3D.h"
#include "Vector4D.h"

namespace TY
{
    namespace ShapeBuilder3D
    {
        struct Vertex3D
        {
            Float3 pos;
            Float4 color;

            void set(const Float3& pos_, const ColorF32& color_);
        };

        using BufferCreator = ShapeBufferCreator<Vertex3D>;

        using index_type = BufferCreator::index_type;

        // -----------------------------------------------

        index_type BuildLine(BufferCreator& bufferCreator, const Shape3D::Line& line);
    }
}
