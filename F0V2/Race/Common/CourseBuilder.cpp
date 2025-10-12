#include "pch.h"
#include "CourseBuilder.h"

#include "CourseConstants.h"
#include "TY/Quaternion.h"
#include "TY/Immediate3D.h"

using namespace Race;

namespace
{
    ModelBuffer buildRoadModel(const CourseSegment& segment, CoursePolygoneCollider* outCollider)
    {
        Array<ModelVertex> vertices((segment.midwayStrips.size() - 1) * 8);
        Array<uint16_t> indices((segment.midwayStrips.size() - 1) * 12);
        int v_offset{};
        int i_offset{};
        for (int m = 0; m < segment.midwayStrips.size() - 1; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            // 表面
            vertices[v_offset] = ModelVertex{s1.leftmost, s1.normal, Float2{}};
            vertices[v_offset + 1] = ModelVertex{s1.rightmost, s1.normal, Float2{1, 0}};
            vertices[v_offset + 2] = ModelVertex{s0.leftmost, s0.normal, Float2{0, 1}};
            vertices[v_offset + 3] = ModelVertex{s0.rightmost, s0.normal, Float2{1, 1}};

            indices[i_offset] = v_offset; // s1.left
            indices[i_offset + 1] = v_offset + 2; // s0.left
            indices[i_offset + 2] = v_offset + 1; // s1.right
            indices[i_offset + 3] = v_offset + 1;
            indices[i_offset + 4] = v_offset + 2;
            indices[i_offset + 5] = v_offset + 3;

            v_offset += 4;
            i_offset += 6;

            // 裏面
            vertices[v_offset] = ModelVertex{s1.leftmost, -s1.normal, Float2{}};
            vertices[v_offset + 1] = ModelVertex{s1.rightmost, -s1.normal, Float2{1, 0}};
            vertices[v_offset + 2] = ModelVertex{s0.leftmost, -s0.normal, Float2{0, 1}};
            vertices[v_offset + 3] = ModelVertex{s0.rightmost, -s0.normal, Float2{1, 1}};

            indices[i_offset] = v_offset; // s1.left
            indices[i_offset + 1] = v_offset + 1; // s0.left
            indices[i_offset + 2] = v_offset + 2; // s1.right
            indices[i_offset + 3] = v_offset + 1;
            indices[i_offset + 4] = v_offset + 3;
            indices[i_offset + 5] = v_offset + 2;

            v_offset += 4;
            i_offset += 6;

            if (outCollider)
            {
                const std::array normals_00_10_01_11{s0.normal, s0.normal, s1.normal, s1.normal};

                outCollider->tris.push_back(IndexedTriangle{
                    s1.leftmost, s0.leftmost, s1.rightmost, outCollider->attributes.size()
                });
                outCollider->attributes.push_back(CourseTriangleAttribute{
                    CourseTriangleAttribute::Triangle_01_00_11,
                    normals_00_10_01_11,
                    s0.rightmost
                });

                outCollider->tris.push_back(IndexedTriangle{
                    s1.rightmost, s0.leftmost, s0.rightmost, outCollider->attributes.size()
                });
                outCollider->attributes.push_back(CourseTriangleAttribute{
                    CourseTriangleAttribute::Triangle_11_00_10,
                    normals_00_10_01_11,
                    s1.leftmost
                });
            }
        }

        ModelMaterial material{};
        material.name = "plain";
        material.parameters.diffuse = Float3::One() * 0.5f;

        ModelShapeBuffer shapeBuffer{
            {ModelShape{std::move(vertices), std::move(indices), 0}}
        };
        ModelBuffer modelBuffer{
            shapeBuffer, {material}
        };

        return modelBuffer;
    }

    struct FaceVertex
    {
        Float3 pos{};
        Float3 normal{};
    };

    void pushFaces(
        Array<ModelVertex>& vertices,
        Array<uint16_t>& indices,
        int& v_offset,
        int& i_offset,
        const FaceVertex& l0,
        const FaceVertex& r0,
        const FaceVertex& l1,
        const FaceVertex& r1,
        CoursePolygoneCollider* outCollider = nullptr
    )
    {
        vertices[v_offset] = ModelVertex{l1.pos, l1.normal, Float2{}};
        vertices[v_offset + 1] = ModelVertex{r1.pos, r1.normal, Float2{1, 0}};
        vertices[v_offset + 2] = ModelVertex{l0.pos, l0.normal, Float2{0, 1}};
        vertices[v_offset + 3] = ModelVertex{r0.pos, r0.normal, Float2{1, 1}};

        indices[i_offset] = v_offset;
        indices[i_offset + 1] = v_offset + 2;
        indices[i_offset + 2] = v_offset + 1;
        indices[i_offset + 3] = v_offset + 1;
        indices[i_offset + 4] = v_offset + 2;
        indices[i_offset + 5] = v_offset + 3;

        v_offset += 4;
        i_offset += 6;

        vertices[v_offset] = ModelVertex{l1.pos, -l1.normal, Float2{}};
        vertices[v_offset + 1] = ModelVertex{r1.pos, -r1.normal, Float2{1, 0}};
        vertices[v_offset + 2] = ModelVertex{l0.pos, -l0.normal, Float2{0, 1}};
        vertices[v_offset + 3] = ModelVertex{r0.pos, -r0.normal, Float2{1, 1}};

        indices[i_offset] = v_offset;
        indices[i_offset + 1] = v_offset + 1;
        indices[i_offset + 2] = v_offset + 2;
        indices[i_offset + 3] = v_offset + 1;
        indices[i_offset + 4] = v_offset + 3;
        indices[i_offset + 5] = v_offset + 2;

        v_offset += 4;
        i_offset += 6;

        if (outCollider)
        {
            const std::array normals_00_10_01_11{l0.normal, /* 10: */ r0.normal, /* 01: */ l1.normal, r1.normal};

            outCollider->tris.push_back(IndexedTriangle{l1.pos, l0.pos, r1.pos, outCollider->attributes.size()});
            outCollider->attributes.push_back(CourseTriangleAttribute{
                CourseTriangleAttribute::Triangle_01_00_11,
                normals_00_10_01_11,
                r0.pos
            });

            outCollider->tris.push_back(IndexedTriangle{r1.pos, l0.pos, r0.pos, outCollider->attributes.size()});
            outCollider->attributes.push_back(CourseTriangleAttribute{
                CourseTriangleAttribute::Triangle_11_00_10,
                normals_00_10_01_11,
                l1.pos
            });
        }
    }

    ModelBuffer buildPipeModel(const CourseSegment& segment, CoursePolygoneCollider* outCollider)
    {
        // TODO: 終端部分の調整

        constexpr int subdivision = PipeSubdivision;
        constexpr int halfSubdivision0 = subdivision / 2;
        constexpr int halfSubdivision1 = halfSubdivision0 + 1;

        int entryStrips = 0; // todo: bool 
        while (entryStrips < segment.midwayStrips.size() &&
            segment.midwayStrips[entryStrips].style != CourseSegmentStyle::Pipe)
        {
            ++entryStrips;
        }

        int exitStrips = 0;
        while (exitStrips < segment.midwayStrips.size() &&
            segment.midwayStrips[segment.midwayStrips.size() - 1 - exitStrips].style != CourseSegmentStyle::Pipe)
        {
            ++exitStrips;
        }

        const int pipeStrips = segment.midwayStrips.size() - entryStrips - exitStrips;

        // -----------------------------------------------

        Array<ModelVertex> vertices(
            entryStrips * ((halfSubdivision1) * 4 * 2) +
            (pipeStrips - 1) * (subdivision * 4 * 2) +
            exitStrips * (halfSubdivision1 * 4 * 2));
        Array<uint16_t> indices(
            entryStrips * (halfSubdivision1 * 6 * 2) +
            (pipeStrips - 1) * (subdivision * 6 * 2) +
            exitStrips * (halfSubdivision1 * 6 * 2));
        int v_offset{};
        int i_offset{};

        // -----------------------------------------------

        constexpr float r = 25.0f; // TODO

        if (entryStrips)
        {
            auto& s0 = segment.midwayStrips[0];
            assert(s0.style != CourseSegmentStyle::Pipe);

            auto& s1 = segment.midwayStrips[1];
            assert(s1.style == CourseSegmentStyle::Pipe);

            for (int i0 = 0; i0 < halfSubdivision1 - 1; ++i0)
            {
                const int i1 = i0 + 1;
                const float t0 = static_cast<float>(i0) / (halfSubdivision1 - 1);
                const float t1 = static_cast<float>(i1) / (halfSubdivision1 - 1);

                FaceVertex l0, r0, l1, r1;

                l0.pos = s0.leftmost * (1 - t0) + s0.rightmost * t0;
                r0.pos = s0.leftmost * (1 - t1) + s0.rightmost * t1;
                l0.normal = s0.normal;
                r0.normal = s0.normal;

                const auto& ringVectors = s1.pipe.ringVectors;
                l1.pos = s1.center + ringVectors[i0] * r;
                r1.pos = s1.center + ringVectors[i1] * r;
                l1.normal = -ringVectors[i0];
                r1.normal = -ringVectors[i1];

                pushFaces(
                    vertices, indices, v_offset, i_offset,
                    l0, r0, l1, r1,
                    outCollider);
            }
        }

        for (int m = entryStrips; m < entryStrips + pipeStrips - 1; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            std::array<Float3, subdivision> n0s = s0.pipe.ringVectors;
            std::array<Float3, subdivision> n1s = s1.pipe.ringVectors;

            // 表面
            for (int i0 = 0; i0 < subdivision; ++i0)
            {
                const int i1 = (i0 + 1) % subdivision;

                FaceVertex l0, r0, l1, r1;

                l0.pos = s0.center + n0s[i0] * r;
                r0.pos = s0.center + n0s[i1] * r;
                l1.pos = s1.center + n1s[i0] * r;
                r1.pos = s1.center + n1s[i1] * r;

                l0.normal = -n0s[i0];
                r0.normal = -n0s[i1];
                l1.normal = -n1s[i0];
                r1.normal = -n1s[i1];

                pushFaces(
                    vertices, indices, v_offset, i_offset,
                    l0, r0, l1, r1,
                    outCollider);
            }
        }

        ModelMaterial material{};
        material.name = "plain";
        material.parameters.diffuse = Float3::One() * 0.5f;

        ModelShapeBuffer shapeBuffer{
            {ModelShape{std::move(vertices), std::move(indices), 0}}
        };
        ModelBuffer modelBuffer{
            shapeBuffer, {material}
        };

        return modelBuffer;
    }
}

namespace Race
{
    ModelBuffer BuildCourseModel(const CourseSegment& segment, CoursePolygoneCollider* outCollider)
    {
        assert(segment.midwayStrips.size() > 0);

        if (segment.style == CourseSegmentStyle::Road)
        {
            return buildRoadModel(segment, outCollider);
        }
        else if (segment.style == CourseSegmentStyle::Pipe)
        {
            return buildPipeModel(segment, outCollider);
        }
        else
        {
            assert(false);
            return {};
        }
    }
}
