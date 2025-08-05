#include "pch.h"
#include "ShapeBuilder2D.h"

#include "Logger.h"

using namespace TY;

namespace
{
    constexpr int maxIndices = 65535;

    constexpr std::array<ShapeBuilder2D::index_type, 6> rectIndexTable = {0, 1, 2, 2, 1, 3};
}

namespace TY
{
    namespace ShapeBuilder2D
    {
        void Vertex2D::set(const Float2& pos_, const ColorF32& color_)
        {
            pos = pos_;
            color = color_.toFloat4();
        }

        void Vertex2D::set(const Float2& pos_, const Float2& tex_, const Float4& color_)
        {
            pos = pos_;
            tex = tex_;
            color = color_;
        }

        bool BufferSpan::isEmpty() const
        {
            return vertices.empty() || indices.empty();
        }

        BufferSpan BufferCreator::request(int vertexCount, int indexCount)
        {
            if (indexCount > maxIndices || vertexCount > maxIndices)
            {
                LogError.writeln("BufferCreator: Requested vertex or index count exceeds maximum allowed size.");
                return {};
            }

            // インデックスバッファの容量が一杯になった場合は次のバッファを使う
            if (m_buffers.empty() ||
                static_cast<int>(m_buffers.back().indices.size()) + indexCount > maxIndices)
            {
                m_buffers.emplace_back();
                // m_buffers.back().indices.reserve(maxIndices);
                // m_buffers.back().vertices.reserve(maxIndices);
            }

            auto& buffer = m_buffers.back();

            const size_t currentVertexCount = buffer.vertices.size();
            const size_t currentIndexCount = buffer.indices.size();

            buffer.vertices.resize(static_cast<int>(buffer.vertices.size()) + vertexCount);
            buffer.indices.resize(static_cast<int>(buffer.indices.size()) + indexCount);

            BufferSpan span;
            span.vertices = {buffer.vertices.data() + currentVertexCount, static_cast<uint16_t>(vertexCount)};
            span.indices = {buffer.indices.data() + currentIndexCount, static_cast<uint16_t>(indexCount)};
            span.indexOffset = static_cast<uint16_t>(currentVertexCount);

            return span;
        }

        void BufferCreator::clear()
        {
            m_buffers.clear();
        }

        const Array<BufferCreator::buffer_type>& BufferCreator::buffers() const
        {
            return m_buffers;
        }

        // -----------------------------------------------

        index_type BuildRetangle(BufferCreator& bufferCreator, const Shape2D::Rectangle& rectangle)
        {
            constexpr int indexSize = rectIndexTable.size();
            const auto buffer = bufferCreator.request(4, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            vertices[0].set(rectangle.rect.tl(), rectangle.colors[0]);
            vertices[1].set(rectangle.rect.tr(), rectangle.colors[1]);
            vertices[2].set(rectangle.rect.bl(), rectangle.colors[2]);
            vertices[3].set(rectangle.rect.br(), rectangle.colors[3]);

            for (int i = 0; i < rectIndexTable.size(); ++i)
            {
                indices[i] = buffer.indexOffset + rectIndexTable[i];
            }

            return indexSize;
        }
    }
}
