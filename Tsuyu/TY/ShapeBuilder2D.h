#pragma once
#include "Array.h"
#include "ArrayPool.h"
#include "Shape2D.h"
#include "Vector2D.h"
#include "Vector4D.h"

namespace TY
{
    namespace ShapeBuilder2D
    {
        struct Vertex2D
        {
            Float2 pos;
            Float2 tex;
            Float4 color;

            void set(const Float2& pos_, const ColorF32& color_);

            void set(const Float2& pos_, const Float2& tex_, const ColorF32& color_);
        };

        using index_type = uint16_t;

        struct BufferSpan
        {
            std::span<Vertex2D> vertices;
            std::span<index_type> indices;
            uint16_t indexOffset;

            bool isEmpty() const;
        };

        class BufferCreator
        {
        public:
            struct buffer_type
            {
                Array<Vertex2D> vertices;
                Array<index_type> indices;
            };

            BufferSpan request(int vertexCount, int indexCount);

            void clear();

            void step();

            const ArrayPool<buffer_type>& buffers() const
            {
                return m_buffers;
            }

        private:
            ArrayPool<buffer_type> m_buffers{};
        };

        // -----------------------------------------------

        index_type BuildRetangle(BufferCreator& bufferCreator, const Shape2D::Rectangle& rectangle);

        index_type BuildLine(BufferCreator& bufferCreator, const Shape2D::Line& line);

        index_type BuildSquareDotLine(BufferCreator& bufferCreator, const Shape2D::SquareDotLine& line, float scale);
    }
}
