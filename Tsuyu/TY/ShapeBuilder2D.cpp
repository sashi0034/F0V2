#include "pch.h"
#include "ShapeBuilder2D.h"

#include "Logger.h"

using namespace TY;

// reference: https://github.com/Siv3D/OpenSiv3D/blob/main/Siv3D/src/Siv3D/Renderer2D/Vertex2DBuilder.cpp

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

        void Vertex2D::set(const Float2& pos_, const Float2& tex_, const ColorF32& color_)
        {
            pos = pos_;
            tex = tex_;
            color = color_.toFloat4();
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
            if (m_buffers.logical_empty() ||
                static_cast<int>(m_buffers.logical_back().indices.size()) + indexCount > maxIndices)
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

            BufferSpan span;
            span.vertices = {buffer.vertices.data() + currentVertexCount, static_cast<uint16_t>(vertexCount)};
            span.indices = {buffer.indices.data() + currentIndexCount, static_cast<uint16_t>(indexCount)};
            span.indexOffset = static_cast<uint16_t>(currentVertexCount);

            return span;
        }

        void BufferCreator::clear()
        {
            m_buffers.logical_resize(0);
        }

        void BufferCreator::step()
        {
            m_buffers.add_logical_size(1);
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

        index_type BuildLine(BufferCreator& bufferCreator, const Shape2D::Line& line)
        {
            if (line.thickness <= 0.0f)
            {
                return 0;
            }

            constexpr int indexSize = rectIndexTable.size();
            const auto buffer = bufferCreator.request(4, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            const float halfThickness = line.thickness * 0.5f;
            const Float2 direction = (line.end - line.start).normalized();
            const Float2 normal{-direction.y * halfThickness, direction.x * halfThickness};

            vertices[0].set(line.start + normal, line.colors[0]);
            vertices[1].set(line.start - normal, line.colors[0]);
            vertices[2].set(line.end + normal, line.colors[1]);
            vertices[3].set(line.end - normal, line.colors[1]);

            for (int i = 0; i < rectIndexTable.size(); ++i)
            {
                indices[i] = buffer.indexOffset + rectIndexTable[i];
            }

            return indexSize;
        }

        index_type BuildSquareDotLine(BufferCreator& bufferCreator, const Shape2D::SquareDotLine& dotLine, float scale)
        {
            const auto& line = dotLine.line;
            if (line.thickness <= 0.0f)
            {
                return 0;
            }

            constexpr index_type vertexSize = 4;
            constexpr index_type indexSize = rectIndexTable.size();

            const auto buffer = bufferCreator.request(vertexSize, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            auto& vertices = buffer.vertices;
            auto& indices = buffer.indices;
            const auto indexOffset = buffer.indexOffset;

            const float halfThickness = line.thickness * 0.5f;
            const Float2 v = (line.end - line.start);
            const float lineLength = v.length();
            const Float2 direction = v / lineLength;
            const Float2 normal{-direction.y * halfThickness, direction.x * halfThickness};
            const Float2 lineHalf = direction * halfThickness;

            const Float2 start2 = line.start - lineHalf;
            const Float2 end2 = line.end + lineHalf;

            const float lineLengthN = lineLength / line.thickness;
            const float uOffset = (1.0f - Math::Fraction(dotLine.dotOffset / 3.0f / line.thickness)) * 3.0f;
            const float vInfo = Min<float>(1.0f / (line.thickness * scale), 1.0f);

            vertices[0].set(start2 + normal, {uOffset, vInfo}, line.colors[0]);
            vertices[1].set(start2 - normal, {uOffset, vInfo}, line.colors[0]);
            vertices[2].set(end2 + normal, {uOffset + lineLengthN, vInfo}, line.colors[1]);
            vertices[3].set(end2 - normal, {uOffset + lineLengthN, vInfo}, line.colors[1]);

            for (index_type i = 0; i < rectIndexTable.size(); ++i)
            {
                indices[i] = indexOffset + rectIndexTable[i];
            }

            return indexSize;
        }
    }
}
