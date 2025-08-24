#pragma once
#include "Array.h"
#include "ArrayPool.h"
#include "Logger.h"

namespace TY
{
    template <typename VertexType>
    struct ShapeBufferSpan
    {
        using index_type = uint16_t;

        std::span<VertexType> vertices;
        std::span<index_type> indices;
        uint16_t indexOffset;

        bool isEmpty() const
        {
            return vertices.empty() || indices.empty();
        }
    };

    template <typename VertexType>
    class ShapeBufferCreator
    {
    public:
        using index_type = ShapeBufferSpan<VertexType>::index_type;

        struct buffer_type
        {
            Array<VertexType> vertices;
            Array<index_type> indices;
        };

        static constexpr int MaxIndices = 65535;

        ShapeBufferSpan<VertexType> request(int vertexCount, int indexCount)
        {
            if (indexCount > MaxIndices || vertexCount > MaxIndices)
            {
                LogError.writeln("BufferCreator: Requested vertex or index count exceeds maximum allowed size.");
                return {};
            }

            // インデックスバッファの容量が一杯になった場合は次のバッファを使う
            if (m_buffers.logical_empty() ||
                static_cast<int>(m_buffers.logical_back().indices.size()) + indexCount > MaxIndices)
            {
                m_buffers.add_logical_size(1);
                m_buffers.logical_back().indices.resize(0);
                m_buffers.logical_back().vertices.resize(0);
            }

            auto& buffer = m_buffers.logical_back();

            const size_t currentVertexCount = buffer.vertices.size();
            const size_t currentIndexCount = buffer.indices.size();

            buffer.vertices.resize(static_cast<int>(buffer.vertices.size()) + vertexCount);
            buffer.indices.resize(static_cast<int>(buffer.indices.size()) + indexCount);

            ShapeBufferSpan<VertexType> span;
            span.vertices = {buffer.vertices.data() + currentVertexCount, static_cast<uint16_t>(vertexCount)};
            span.indices = {buffer.indices.data() + currentIndexCount, static_cast<uint16_t>(indexCount)};
            span.indexOffset = static_cast<uint16_t>(currentVertexCount);

            return span;
        }

        void clear()
        {
            m_buffers.logical_resize(0);
        }

        void step()
        {
            m_buffers.add_logical_size(1);
        }

        const ArrayPool<buffer_type>& buffers() const
        {
            return m_buffers;
        }

    private:
        ArrayPool<buffer_type> m_buffers{};
    };
}
