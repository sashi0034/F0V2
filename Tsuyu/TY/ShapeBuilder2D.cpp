#include "pch.h"
#include "ShapeBuilder2D.h"

#include "Logger.h"

using namespace TY;

// reference: https://github.com/Siv3D/OpenSiv3D/blob/main/Siv3D/src/Siv3D/Renderer2D/Vertex2DBuilder.cpp

namespace
{
    constexpr int maxIndices = 65535;

    constexpr std::array<ShapeBuilder2D::index_type, 6> rectIndexTable = {0, 1, 2, 2, 1, 3}; // 1-2 対角線

    constexpr std::array<ShapeBuilder2D::index_type, 6> rectIndexTable02 = {0, 1, 2, 2, 0, 3}; // 0-2 対角線

    const std::array<ShapeBuilder2D::index_type, 6>& takeRectIndexTable(
        std::span<const ShapeBuilder2D::Vertex2D> vertexes)
    {
        assert(vertexes.size() == 4);

        const Float2 v01 = vertexes[1].pos - vertexes[0].pos;
        const Float2 v02 = vertexes[2].pos - vertexes[0].pos;
        const Float2 v03 = vertexes[3].pos - vertexes[0].pos;

        return Math::Sign(v01.cross(v02)) == Math::Sign(v02.cross(v03))
                   ? rectIndexTable02
                   : rectIndexTable;
    }
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

        index_type BuildPath(BufferCreator& bufferCreator, const Shape2D::Path& path)
        {
            if (path.thickness <= 0.0f)
            {
                return 0;
            }

            const int vertexCount = 4 + 3 * (path.points.size() - 2);
            if (vertexCount <= 0)
            {
                return 0;
            }

            const int indexCount = 6 * (path.points.size() - 1) + 3 * (path.points.size() - 2);

            const auto buffer = bufferCreator.request(vertexCount, indexCount);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            const float halfThickness = path.thickness * 0.5f;
            const auto& color = path.color;

            int vertexHead = 0;
            int indexHead = 0;

            for (int i = 0; i < path.points.size() - 2; ++i)
            {
                const Float2& p = path.points[i];
                const Float2& m = path.points[i + 1];
                const Float2& q = path.points[i + 2];;

                const Float2 mp_dir = (p - m).normalized();
                const Float2 mq_dir = (q - m).normalized();

                Float2 mp_normal = Float2{-mp_dir.y, mp_dir.x};
                if (mp_normal.dot(mq_dir) < 0.0f)
                {
                    mp_normal = -mp_normal;
                }

                Float2 mq_normal = Float2{-mq_dir.y, mq_dir.x};
                if (mq_normal.dot(mp_dir) <= 0.0f) // k = 0 の場合のためにこちらは等号をつける
                {
                    mq_normal = -mq_normal;
                }

                float k;
                if (Abs(mp_dir.x - mq_dir.x) > 1e-5)
                {
                    // 連立方程式の結果より
                    k = -halfThickness * (mp_normal.x - mq_normal.x) / (mp_dir.x - mq_dir.x);
                }
                else
                {
                    k = 0;
                }

                const Float2 intersect = m + k * mp_dir + halfThickness * mp_normal;

                // -----------------------------------------------

                if (i == 0)
                {
                    Float2 p0 = p - mp_normal * halfThickness;
                    Float2 p1 = p + mp_normal * halfThickness;

                    assert(vertexHead == 0);
                    vertices[vertexHead + 0].set(p0, color);
                    vertices[vertexHead + 1].set(p1, color);
                    vertexHead += 2;
                }

                vertices[vertexHead + 0].set(m - mp_normal * halfThickness, color);
                vertices[vertexHead + 1].set(intersect, color);
                vertices[vertexHead + 2].set(m - mq_normal * halfThickness, color);

                // 前パスの四角形
                const auto& table = takeRectIndexTable(vertices.subspan(vertexHead - 2, 4));
                for (int id = 0; id < 6; ++id)
                {
                    indices[indexHead + id] = buffer.indexOffset + vertexHead - 2 + table[id];
                }

                // 中間地点の三角形
                indices[indexHead + 6] = buffer.indexOffset + vertexHead + 0;
                indices[indexHead + 7] = buffer.indexOffset + vertexHead + 1;
                indices[indexHead + 8] = buffer.indexOffset + vertexHead + 2;

                vertexHead += 3;
                indexHead += 9;

                if (i == path.points.size() - 3)
                {
                    // 終点部分の四角形
                    assert(vertexHead == vertexCount - 2);
                    assert(indexHead == indexCount - 6);

                    Float2 q0 = q + mq_normal * halfThickness;
                    Float2 q1 = q - mq_normal * halfThickness;

                    vertices[vertexHead + 0].set(q0, color);
                    vertices[vertexHead + 1].set(q1, color);

                    const auto& table2 = takeRectIndexTable(vertices.subspan(vertexHead - 2, 4));
                    for (int id = 0; id < 6; ++id)
                    {
                        indices[indexHead + id] = buffer.indexOffset + vertexHead - 2 + table2[id];
                    }

                    vertexHead += 2;
                    indexHead += 6;
                }
            }

            return indexCount;
        }
    }
}
