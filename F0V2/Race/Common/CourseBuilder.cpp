#include "pch.h"
#include "CourseBuilder.h"

#include "CourseConstants.h"
#include "TY/Quaternion.h"
#include "TY/Immediate3D.h"

using namespace Race;

namespace
{
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
            const std::array normals_00_10_01_11{
                /* 00: */ l0.normal, /* 10: */ r0.normal, /* 01: */ l1.normal, /* 11: */ r1.normal
            };

            // 01 +-----+ 11
            //    |\    |
            //    | \   |
            //    |  C  |
            //    | D \ |
            //    |    \|
            // 00 +-----+ 10

            const Float3& p00 = l0.pos;
            const Float3& p10 = r0.pos;
            const Float3& p01 = l1.pos;
            const Float3& p11 = r1.pos;

            const Float3 C = (p10 + p01) * 0.5f;
            const Float3 D = (p00 + p10 + p01 + p11) * 0.25f;
            const Float3 CD = D - C;

            const Float3 N = l0.normal + r0.normal + l1.normal + r1.normal;

            // 双曲面が二つの三角形の上側に張るようにする
            if (CD.dot(N) >= 0)
            {
                // 10-01 対角線
                outCollider->tris.push_back(IndexedTriangle{p10, p01, p00, outCollider->attributes.size()});
                outCollider->attributes.push_back(CourseTriangleAttribute{
                    CourseTriangleAttribute::Triangle_10_01_00,
                    normals_00_10_01_11,
                    p11
                });

                outCollider->tris.push_back(IndexedTriangle{p10, p11, p01, outCollider->attributes.size()});
                outCollider->attributes.push_back(CourseTriangleAttribute{
                    CourseTriangleAttribute::Triangle_10_11_01,
                    normals_00_10_01_11,
                    p00
                });
            }
            else
            {
                // 00-11 対角線
                outCollider->tris.push_back(IndexedTriangle{p00, p10, p11, outCollider->attributes.size()});
                outCollider->attributes.push_back(CourseTriangleAttribute{
                    CourseTriangleAttribute::Triangle_00_10_11,
                    normals_00_10_01_11,
                    p01
                });

                outCollider->tris.push_back(IndexedTriangle{p00, p11, p01, outCollider->attributes.size()});
                outCollider->attributes.push_back(CourseTriangleAttribute{
                    CourseTriangleAttribute::Triangle_00_11_01,
                    normals_00_10_01_11,
                    p10
                });
            }
        }
    }

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

            const FaceVertex l0{s0.leftmost, s0.normal};
            const FaceVertex r0{s0.rightmost, s0.normal};
            const FaceVertex l1{s1.leftmost, s1.normal};
            const FaceVertex r1{s1.rightmost, s1.normal};

            pushFaces(
                vertices, indices, v_offset, i_offset,
                l0, r0, l1, r1,
                outCollider);
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

    ModelBuffer buildPipeModel(const CourseSegment& segment, CoursePolygoneCollider* outCollider)
    {
        // TODO: 終端部分の調整

        constexpr int subdivision = PipeSubdivision;
        constexpr int halfSubdivision0 = subdivision / 2;
        constexpr int halfSubdivision1 = halfSubdivision0 + 1;

        const int hasEntry = segment.midwayStrips.size() >= PipeEntryExitStrips &&
            segment.midwayStrips[0].style != CourseSegmentStyle::Pipe;

        const bool hasExit = segment.midwayStrips.size() >= PipeEntryExitStrips &&
            segment.midwayStrips[segment.midwayStrips.size() - 1].style != CourseSegmentStyle::Pipe;

        const int pipeStrips = segment.midwayStrips.size() - (hasEntry + hasExit) * PipeEntryExitStrips;

        // -----------------------------------------------

        Array<ModelVertex> vertices(
            (hasEntry + hasExit) * PipeEntryExitStrips * ((halfSubdivision1) * 4 * 2) +
            (pipeStrips - 1) * (subdivision * 4 * 2));
        Array<uint16_t> indices(
            (hasEntry + hasExit) * PipeEntryExitStrips * (halfSubdivision1 * 6 * 2) +
            (pipeStrips - 1) * (subdivision * 6 * 2));
        int v_offset{};
        int i_offset{};

        // -----------------------------------------------

        constexpr float r = 25.0f; // TODO

        if (hasEntry)
        {
            auto& s0 = segment.midwayStrips[0];
            assert(s0.style != CourseSegmentStyle::Pipe);

            auto& s1 = segment.midwayStrips[PipeEntryExitStrips];
            assert(s1.style == CourseSegmentStyle::Pipe);

            for (int i0 = 0; i0 < halfSubdivision1 - 1; ++i0)
            {
                const int i1 = i0 + 1;
                const float t0 = static_cast<float>(i0) / (halfSubdivision1 - 1);
                const float t1 = static_cast<float>(i1) / (halfSubdivision1 - 1);

                FaceVertex cap_l0, cap_r0, cap_l1, cap_r1;

                cap_l0.pos = s0.leftmost * (1 - t0) + s0.rightmost * t0;
                cap_r0.pos = s0.leftmost * (1 - t1) + s0.rightmost * t1;
                cap_l0.normal = s0.normal;
                cap_r0.normal = s0.normal;

                const auto& ringVectors = s1.pipe.ringVectors;
                cap_l1.pos = s1.center + ringVectors[i0] * r;
                cap_r1.pos = s1.center + ringVectors[i1] * r;
                cap_l1.normal = -ringVectors[i0];
                cap_r1.normal = -ringVectors[i1];

                for (int s = 0; s < PipeEntryExitStrips; ++s)
                {
                    const float s0_rate = static_cast<float>(s) / PipeEntryExitStrips;
                    const float r1_rate = static_cast<float>(s + 1) / PipeEntryExitStrips;
                    FaceVertex l0, r0, l1, r1;
                    l0.pos = cap_l0.pos * (1 - s0_rate) + cap_l1.pos * s0_rate;
                    r0.pos = cap_r0.pos * (1 - s0_rate) + cap_r1.pos * s0_rate;
                    l1.pos = cap_l0.pos * (1 - r1_rate) + cap_l1.pos * r1_rate;
                    r1.pos = cap_r0.pos * (1 - r1_rate) + cap_r1.pos * r1_rate;
                    l0.normal = (cap_l0.normal * (1 - s0_rate) + cap_l1.normal * s0_rate).normalized();
                    r0.normal = (cap_r0.normal * (1 - s0_rate) + cap_r1.normal * s0_rate).normalized();
                    l1.normal = (cap_l0.normal * (1 - r1_rate) + cap_l1.normal * r1_rate).normalized();
                    r1.normal = (cap_r0.normal * (1 - r1_rate) + cap_r1.normal * r1_rate).normalized();

                    pushFaces(
                        vertices, indices, v_offset, i_offset,
                        l0, r0, l1, r1,
                        outCollider);
                }
            }
        }

        for (int m = hasEntry * PipeEntryExitStrips; m < hasEntry * PipeEntryExitStrips + pipeStrips - 1; ++m)
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

        if (hasExit)
        {
            auto& s0 = segment.midwayStrips[segment.midwayStrips.size() - 1 - PipeEntryExitStrips];
            assert(s0.style == CourseSegmentStyle::Pipe);

            auto& s1 = segment.midwayStrips[segment.midwayStrips.size() - 1];
            assert(s1.style != CourseSegmentStyle::Pipe);

            for (int i0 = 0; i0 < halfSubdivision1 - 1; ++i0)
            {
                const int i1 = i0 + 1;
                const float t0 = static_cast<float>(i0) / (halfSubdivision1 - 1);
                const float t1 = static_cast<float>(i1) / (halfSubdivision1 - 1);

                FaceVertex cap_l0, cap_r0, cap_l1, cap_r1;

                const auto& ringVectors = s0.pipe.ringVectors;
                cap_l0.pos = s0.center + ringVectors[i0] * r;
                cap_r0.pos = s0.center + ringVectors[i1] * r;
                cap_l0.normal = -ringVectors[i0];
                cap_r0.normal = -ringVectors[i1];

                cap_l1.pos = s1.leftmost * (1 - t0) + s1.rightmost * t0;
                cap_r1.pos = s1.leftmost * (1 - t1) + s1.rightmost * t1;
                cap_l1.normal = s1.normal;
                cap_r1.normal = s1.normal;

                for (int s = 0; s < PipeEntryExitStrips; ++s)
                {
                    const float s0_rate = static_cast<float>(s) / PipeEntryExitStrips;
                    const float r1_rate = static_cast<float>(s + 1) / PipeEntryExitStrips;
                    FaceVertex l0, r0, l1, r1;
                    l0.pos = cap_l0.pos * (1 - s0_rate) + cap_l1.pos * s0_rate;
                    r0.pos = cap_r0.pos * (1 - s0_rate) + cap_r1.pos * s0_rate;
                    l1.pos = cap_l0.pos * (1 - r1_rate) + cap_l1.pos * r1_rate;
                    r1.pos = cap_r0.pos * (1 - r1_rate) + cap_r1.pos * r1_rate;
                    l0.normal = (cap_l0.normal * (1 - s0_rate) + cap_l1.normal * s0_rate).normalized();
                    r0.normal = (cap_r0.normal * (1 - s0_rate) + cap_r1.normal * s0_rate).normalized();
                    l1.normal = (cap_l0.normal * (1 - r1_rate) + cap_l1.normal * r1_rate).normalized();
                    r1.normal = (cap_r0.normal * (1 - r1_rate) + cap_r1.normal * r1_rate).normalized();

                    pushFaces(
                        vertices, indices, v_offset, i_offset,
                        l0, r0, l1, r1,
                        outCollider);
                }
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
