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

        index_type BuildLineSet(BufferCreator& bufferCreator, const Shape3D::LineSet& lineSet)
        {
            const int lineCount = static_cast<int>(lineSet.lines.size());
            const int indexSize = lineCount * 2;
            const auto buffer = bufferCreator.request(lineCount * 2, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            for (int i = 0; i < lineCount; ++i)
            {
                const auto& [start, end] = lineSet.lines[i];
                vertices[i * 2 + 0].set(start, lineSet.colors[0]);
                vertices[i * 2 + 1].set(end, lineSet.colors[1]);
            }

            for (int i = 0; i < indexSize; ++i)
            {
                indices[i] = buffer.indexOffset + i;
            }

            return indexSize;
        }
    }
}
