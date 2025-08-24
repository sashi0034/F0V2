#include "pch.h"
#include "ShapeBuilder3D.h"

namespace TY
{
    namespace ShapeBuilder3D
    {
        void Vertex3D::set(const Float3& pos_, const ColorF32& color_)
        {
            pos = pos_;
            color = color_.toFloat4();
        }

        index_type BuildLine(BufferCreator& bufferCreator, const Shape3D::Line& line)
        {
            constexpr int indexSize = 2;
            const auto buffer = bufferCreator.request(2, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            vertices[0].set(line.start, line.colors[0]);
            vertices[1].set(line.end, line.colors[1]);

            for (int i = 0; i < 2; ++i)
            {
                indices[i] = buffer.indexOffset + i;
            }

            return indexSize;
        }
    }
}
