#include "pch.h"
#include "ImmediateBuilder2D.h"

using namespace TY;

// reference: https://github.com/Siv3D/OpenSiv3D/blob/main/Siv3D/src/Siv3D/Renderer2D/Vertex2DBuilder.cpp

namespace
{
    constexpr std::array<ImmediateBuilder2D::index_type, 6> rectIndexTable = {0, 1, 2, 2, 1, 3}; // 1-2 対角線

    constexpr std::array<ImmediateBuilder2D::index_type, 6> rectIndexTable02 = {0, 1, 2, 2, 0, 3}; // 0-2 対角線

    const std::array<ImmediateBuilder2D::index_type, 6>& takeRectIndexTable(
        const ImmediateBuilder2D::Vertex2D& v0,
        const ImmediateBuilder2D::Vertex2D& v1,
        const ImmediateBuilder2D::Vertex2D& v2,
        const ImmediateBuilder2D::Vertex2D& v3)
    {
        const Float2 v01 = v1.pos - v0.pos;
        const Float2 v02 = v2.pos - v0.pos;
        const Float2 v03 = v3.pos - v0.pos;

        return Math::Sign(v01.cross(v02)) == Math::Sign(v02.cross(v03))
                   ? rectIndexTable02
                   : rectIndexTable;
    }

    const std::array<ImmediateBuilder2D::index_type, 6>& takeRectIndexTable(
        std::span<const ImmediateBuilder2D::Vertex2D> vertexes)
    {
        assert(vertexes.size() == 4);
        return takeRectIndexTable(vertexes[0], vertexes[1], vertexes[2], vertexes[3]);
    }

    // -----------------------------------------------
}

namespace TY
{
    namespace ImmediateBuilder2D
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

        // -----------------------------------------------

        index_type BuildRect_Standard(BufferCreator& bufferCreator, const Immediate2D::Rect& rect)
        {
            constexpr int indexSize = rectIndexTable.size();
            const auto buffer = bufferCreator.request(4, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            vertices[0].set(rect.rect.tl(), rect.colors[0]);
            vertices[1].set(rect.rect.tr(), rect.colors[1]);
            vertices[2].set(rect.rect.bl(), rect.colors[2]);
            vertices[3].set(rect.rect.br(), rect.colors[3]);

            for (int i = 0; i < rectIndexTable.size(); ++i)
            {
                indices[i] = buffer.indexOffset + rectIndexTable[i];
            }

            return indexSize;
        }

        index_type BuildRect_Outline(BufferCreator& bufferCreator, const Immediate2D::Rect& rect)
        {
            constexpr int indexSize = rectIndexTable02.size() + 3 * 8;
            const auto buffer = bufferCreator.request(12, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            vertices[0].set(rect.rect.tl(), rect.colors[0]);
            vertices[1].set(rect.rect.tr(), rect.colors[1]);
            vertices[2].set(rect.rect.br(), rect.colors[3]);
            vertices[3].set(rect.rect.bl(), rect.colors[2]);

            for (int i = 0; i < rectIndexTable02.size(); ++i)
            {
                indices[i] = buffer.indexOffset + rectIndexTable02[i];
            }

            // -----------------------------------------------

            vertices[4].set(rect.rect.tl(), rect.outline.innerColor);
            vertices[5].set(rect.rect.tr(), rect.outline.innerColor);
            vertices[6].set(rect.rect.br(), rect.outline.innerColor);
            vertices[7].set(rect.rect.bl(), rect.outline.innerColor);

            const RectF outlineRect = rect.rect.stretched(rect.outline.thickness);
            vertices[8].set(outlineRect.tl(), rect.outline.outerColor);
            vertices[9].set(outlineRect.tr(), rect.outline.outerColor);
            vertices[10].set(outlineRect.br(), rect.outline.outerColor);
            vertices[11].set(outlineRect.bl(), rect.outline.outerColor);

            for (int i = 0; i < 4; ++i)
            {
                const int offset = 6 + i * 3;
                indices[offset + 0] = buffer.indexOffset + 8 + i;
                indices[offset + 1] = buffer.indexOffset + 4 + (i + 1) % 4;
                indices[offset + 2] = buffer.indexOffset + 4 + i;
            }

            for (int i = 0; i < 4; ++i)
            {
                const int offset = 6 + (i + 4) * 3;
                indices[offset + 0] = buffer.indexOffset + 8 + i;
                indices[offset + 1] = buffer.indexOffset + 8 + (i + 1) % 4;
                indices[offset + 2] = buffer.indexOffset + 4 + (i + 1) % 4;
            }

            return indexSize;
        }

        index_type BuildRect(BufferCreator& bufferCreator, const Immediate2D::Rect& rect)
        {
            if (rect.outline.thickness <= 0.0f)
            {
                return BuildRect_Standard(bufferCreator, rect);
            }
            else
            {
                return BuildRect_Outline(bufferCreator, rect);
            }
        }

        Array<Float2>& BuildRoundRect_offsetTable(int segments)
        {
            static std::unordered_map<int, Array<Float2>> s_offsetTable{};
            if (not s_offsetTable.contains(segments))
            {
                for (int i = 0; i < 4; ++i)
                {
                    for (int s = 0; s < segments + 2; ++s)
                    {
                        const float angle =
                            Math::PiF + (Math::HalfPiF * (i + static_cast<float>(s) / (segments + 1.0f)));
                        const Float2 offset{std::cos(angle), std::sin(angle)};
                        s_offsetTable[segments].push_back(offset);
                    }
                }
            }

            return s_offsetTable[segments];
        }

        index_type BuildRoundRect_Standard(BufferCreator& bufferCreator, const Immediate2D::RoundRect& rect)
        {
            const int indexSize = (2 + rect.segments) * 3 * 4;
            const auto buffer = bufferCreator.request(1 + (2 + rect.segments) * 4, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            // 中心座標
            vertices[0].set(rect.rect.center(), rect.color);

            const float roundness = Min<float>(rect.roundness, rect.rect.size.minComponent() * 0.5f);

            static constexpr std::array corners = {Float2{0, 0}, Float2{1, 0}, Float2{1, 1}, Float2{0, 1}};

            const Float2* nextOffset = &BuildRoundRect_offsetTable(rect.segments)[0];

            // 左上、右上、右下、左下の順番で処理を行う
            for (int i = 0; i < 4; ++i)
            {
                const auto conerPosition = rect.rect.getRelativePoint(corners[i]);
                const auto dir = corners[i] * 2 - Point{1, 1};
                const auto pivot = conerPosition - dir * roundness;

                for (int s = 0; s < rect.segments + 2; ++s)
                {
#if 0
                    const float angle =
                        Math::PiF + (Math::HalfPiF * (i + static_cast<float>(s) / (rect.segments + 1.0f)));
                    const Float2 offset{std::cos(angle), std::sin(angle)};
#else
                    const Float2 offset = *nextOffset;
                    nextOffset++;
#endif
                    vertices[(2 + rect.segments) * i + 1 + s].set(pivot + offset * roundness, rect.color);
                }
            }

            for (int i = 0; i < indexSize / 3; ++i)
            {
                indices[i * 3 + 0] = buffer.indexOffset + 0;
                indices[i * 3 + 1] = buffer.indexOffset + (i + 0) + 1;
                indices[i * 3 + 2] = buffer.indexOffset + (i + 1) % (indexSize / 3) + 1;
            }

            return indexSize;
        }

        index_type BuildRoundRect_Outline(BufferCreator& bufferCreator, const Immediate2D::RoundRect& rect)
        {
            const int indexSize = (2 + rect.segments) * 3 * 4 * 3;
            const auto buffer = bufferCreator.request(1 + (2 + rect.segments) * 4 * 3, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            // 中心座標
            vertices[0].set(rect.rect.center(), rect.color);

            const float roundness = Min<float>(rect.roundness, rect.rect.size.minComponent() * 0.5f);

            static constexpr std::array corners = {Float2{0, 0}, Float2{1, 0}, Float2{1, 1}, Float2{0, 1}};

            const Float2* nextOffset = &BuildRoundRect_offsetTable(rect.segments)[0];

            // 左上、右上、右下、左下の順番で処理を行う
            for (int i = 0; i < 4; ++i)
            {
                const auto conerPosition = rect.rect.getRelativePoint(corners[i]);
                const auto dir = corners[i] * 2 - Point{1, 1};
                const auto pivot = conerPosition - dir * roundness;

                for (int s = 0; s < rect.segments + 2; ++s)
                {
                    const Float2 offset = *nextOffset;
                    nextOffset++;

                    const Float2 pos = pivot + offset * roundness;
                    vertices[(2 + rect.segments) * i + 1 + s].set(pos, rect.color);
                    vertices[(2 + rect.segments) * (4 + i) + 1 + s].set(pos, rect.outline.innerColor);

                    const Float2 pos2 = pivot + offset * (roundness + rect.outline.thickness);
                    vertices[(2 + rect.segments) * (8 + i) + 1 + s].set(pos2, rect.outline.outerColor);
                }
            }

            const int secondCycleStart = buffer.indexOffset + (2 + rect.segments) * 4;
            const int thirdCycleStart = buffer.indexOffset + (2 + rect.segments) * 8;
            const int indicesPerCycle = (2 + rect.segments) * 3 * 4;
            for (int i = 0; i < indicesPerCycle / 3; ++i)
            {
                indices[i * 3 + 0] = buffer.indexOffset + 0;
                indices[i * 3 + 1] = buffer.indexOffset + (i + 0) + 1;
                indices[i * 3 + 2] = buffer.indexOffset + (i + 1) % (indicesPerCycle / 3) + 1;

                indices[indicesPerCycle + i * 3 + 0] = thirdCycleStart + i + 1;
                indices[indicesPerCycle + i * 3 + 1] = secondCycleStart + i + 1;
                indices[indicesPerCycle + i * 3 + 2] = secondCycleStart + (i + 1) % (indicesPerCycle / 3) + 1;

                indices[indicesPerCycle * 2 + i * 3 + 0] = thirdCycleStart + i + 1;
                indices[indicesPerCycle * 2 + i * 3 + 1] = thirdCycleStart + (i + 1) % (indicesPerCycle / 3) + 1;
                indices[indicesPerCycle * 2 + i * 3 + 2] = secondCycleStart + (i + 1) % (indicesPerCycle / 3) + 1;
            }

            return indexSize;
        }

        index_type BuildRoundRect(BufferCreator& bufferCreator, const Immediate2D::RoundRect& rect)
        {
            if (rect.outline.thickness <= 0.0f)
            {
                return BuildRoundRect_Standard(bufferCreator, rect);
            }
            else
            {
                return BuildRoundRect_Outline(bufferCreator, rect);
            }
        }

        index_type BuildLine(BufferCreator& bufferCreator, const Immediate2D::Line& line)
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

        index_type BuildSquareDotLine(BufferCreator& bufferCreator, const Immediate2D::SquareDotLine& dotLine,
                                      float scale)
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

        index_type BuildPath(BufferCreator& bufferCreator, const Immediate2D::Path& path)
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
                const Float2& q = path.points[i + 2];

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
                const Float2 d_qp = mp_dir - mq_dir;
                if (Abs(d_qp.x) > Abs(d_qp.y) && d_qp.x != 0)
                {
                    // 連立方程式の結果より
                    k = -halfThickness * (mp_normal.x - mq_normal.x) / d_qp.x;
                }
                else if (d_qp.y != 0)
                {
                    k = -halfThickness * (mp_normal.y - mq_normal.y) / d_qp.y;
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

        index_type BuildCyclePath(BufferCreator& bufferCreator, const Immediate2D::CyclePath& cyclePath)
        {
            const auto& path = cyclePath.path;
            if (path.thickness <= 0.0f)
            {
                return 0;
            }

            const int vertexCount = 3 * path.points.size();
            if (vertexCount <= 0)
            {
                return 0;
            }

            const int indexCount = 9 * path.points.size();

            const auto buffer = bufferCreator.request(vertexCount, indexCount);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            const float halfThickness = path.thickness * 0.5f;
            const auto& color = path.color;

            for (int i = 0; i < path.points.size(); ++i)
            {
                const Float2& p = path.points[i];
                const Float2& m = path.points[(i + 1) % path.points.size()];
                const Float2& q = path.points[(i + 2) % path.points.size()];

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
                const Float2 d_qp = mp_dir - mq_dir;
                if (Abs(d_qp.x) > Abs(d_qp.y) && d_qp.x != 0)
                {
                    // 連立方程式の結果より
                    k = -halfThickness * (mp_normal.x - mq_normal.x) / d_qp.x;
                }
                else if (d_qp.y != 0)
                {
                    k = -halfThickness * (mp_normal.y - mq_normal.y) / d_qp.y;
                }
                else
                {
                    k = 0;
                }

                const Float2 intersect = m + k * mp_dir + halfThickness * mp_normal;

                // -----------------------------------------------

                const int vertexHead = i * 3;
                vertices[vertexHead + 0].set(m - mp_normal * halfThickness, color);
                vertices[vertexHead + 1].set(intersect, color);
                vertices[vertexHead + 2].set(m - mq_normal * halfThickness, color);
            }

            for (int i = 0; i < path.points.size(); ++i)
            {
                const int vertexHead = i * 3;
                const int indexHead = i * 9;

                // 前パスの四角形
                if (i == 0)
                {
                    const auto& table = takeRectIndexTable(
                        vertices[vertexCount - 2],
                        vertices[vertexCount - 1],
                        vertices[vertexHead + 0],
                        vertices[vertexHead + 1]);
                    for (int id = 0; id < 6; ++id)
                    {
                        indices[indexHead + id] = buffer.indexOffset + (-2 + table[id] + vertexCount) % vertexCount;
                    }
                }
                else
                {
                    const auto& table = takeRectIndexTable(vertices.subspan(vertexHead - 2, 4));
                    for (int id = 0; id < 6; ++id)
                    {
                        indices[indexHead + id] = buffer.indexOffset + vertexHead - 2 + table[id];
                    }
                }

                // 中間地点の三角形
                indices[indexHead + 6] = buffer.indexOffset + vertexHead + 0;
                indices[indexHead + 7] = buffer.indexOffset + vertexHead + 1;
                indices[indexHead + 8] = buffer.indexOffset + vertexHead + 2;
            }

            return indexCount;
        }

        index_type BuildTexture(BufferCreator& bufferCreator, const Immediate2D::Texture& texture)
        {
            constexpr int indexSize = rectIndexTable.size();
            const auto buffer = bufferCreator.request(4, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            const Float2 drawSize = texture.texture.size() * texture.scale;
            Float2 tl = texture.position;
            tl = tl - drawSize * texture.pivot;

            const Float2 br = tl + drawSize; // FIXME? - Float2{1.0f, 1.0f};

            vertices[0].set(tl, texture.uvRect.tl(), texture.color);
            vertices[1].set({br.x, tl.y}, texture.uvRect.tr(), texture.color);
            vertices[2].set({tl.x, br.y}, texture.uvRect.bl(), texture.color);
            vertices[3].set(br, texture.uvRect.br(), texture.color);

            for (int i = 0; i < rectIndexTable.size(); ++i)
            {
                indices[i] = buffer.indexOffset + rectIndexTable[i];
            }

            return indexSize;
        }

        index_type BuildText(BufferCreator& bufferCreator, const Immediate2D::Text& text)
        {
            const int characterCount = text.text.size();

            const int indexSize = characterCount * 6;
            const auto buffer = bufferCreator.request(characterCount * 4, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            // TODO: 改行など

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            int vertexOffset{};
            Float2 offsetToApply{};
            text.build(
                [&](const Immediate2D::Text::build_intermediate& next)
                {
                    vertices[vertexOffset + 0].set(
                        next.posTL, next.uvTL, text.color);
                    vertices[vertexOffset + 1].set(
                        {next.posBR.x, next.posTL.y}, {next.uvBR.x, next.uvTL.y}, text.color);
                    vertices[vertexOffset + 2].set(
                        {next.posTL.x, next.posBR.y}, {next.uvTL.x, next.uvBR.y}, text.color);
                    vertices[vertexOffset + 3].set(
                        next.posBR, next.uvBR, text.color);
                    vertexOffset += 4;
                },
                offsetToApply);

            for (int i = 0; i < characterCount; ++i)
            {
                for (int j = 0; j < 6; ++j)
                {
                    indices[i * 6 + j] = buffer.indexOffset + i * 4 + rectIndexTable[j];
                }
            }

            if (not text.pivot.isZero())
            {
                for (auto& c : buffer.vertices)
                {
                    c.pos += offsetToApply;
                }
            }

            return indexSize;
        }

        index_type BuildCachedText(BufferCreator& bufferCreator, const Immediate2D::CachedText& text)
        {
            const int characterCount = text.characters.size();

            const int indexSize = characterCount * 6;
            const auto buffer = bufferCreator.request(characterCount * 4, indexSize);
            if (buffer.isEmpty())
            {
                return 0;
            }

            const auto& vertices = buffer.vertices;
            const auto& indices = buffer.indices;

            for (int i = 0; i < characterCount; ++i)
            {
                vertices[i * 4 + 0].set(text.characters[i].posTL, text.characters[i].uvTL, text.color);
                vertices[i * 4 + 1].set({text.characters[i].posBR.x, text.characters[i].posTL.y},
                                        {text.characters[i].uvBR.x, text.characters[i].uvTL.y},
                                        text.color);
                vertices[i * 4 + 2].set({text.characters[i].posTL.x, text.characters[i].posBR.y},
                                        {text.characters[i].uvTL.x, text.characters[i].uvBR.y},
                                        text.color);
                vertices[i * 4 + 3].set(text.characters[i].posBR, text.characters[i].uvBR, text.color);
            }

            for (int i = 0; i < characterCount; ++i)
            {
                for (int j = 0; j < 6; ++j)
                {
                    indices[i * 6 + j] = buffer.indexOffset + i * 4 + rectIndexTable[j];
                }
            }

            return indexSize;
        }
    }
}
