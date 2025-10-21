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

    void pushGroundFaces(
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
                outCollider->groundTris.push_back(IndexedTriangle{p10, p01, p00, outCollider->groundAttrs.size()});
                outCollider->groundAttrs.push_back(GroundTriangleAttribute{
                    GroundTriangleAttribute::Triangle_10_01_00,
                    normals_00_10_01_11,
                    p11
                });

                outCollider->groundTris.push_back(IndexedTriangle{p10, p11, p01, outCollider->groundAttrs.size()});
                outCollider->groundAttrs.push_back(GroundTriangleAttribute{
                    GroundTriangleAttribute::Triangle_10_11_01,
                    normals_00_10_01_11,
                    p00
                });
            }
            else
            {
                // 00-11 対角線
                outCollider->groundTris.push_back(IndexedTriangle{p00, p10, p11, outCollider->groundAttrs.size()});
                outCollider->groundAttrs.push_back(GroundTriangleAttribute{
                    GroundTriangleAttribute::Triangle_00_10_11,
                    normals_00_10_01_11,
                    p01
                });

                outCollider->groundTris.push_back(IndexedTriangle{p00, p11, p01, outCollider->groundAttrs.size()});
                outCollider->groundAttrs.push_back(GroundTriangleAttribute{
                    GroundTriangleAttribute::Triangle_00_11_01,
                    normals_00_10_01_11,
                    p10
                });
            }
        }
    }

    void pushBarrierFaces(
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
            outCollider->gimmickTris.push_back(IndexedTriangle{
                l1.pos, l0.pos, r1.pos, outCollider->gimmickAttrs.size()
            });
            outCollider->gimmickAttrs.push_back(GimmickTriangleAttribute{
                GimmickTriangleAttribute::kind_t::Wall
            });

            outCollider->gimmickTris.push_back(IndexedTriangle{
                r1.pos, l0.pos, r0.pos, outCollider->gimmickAttrs.size()
            });
            outCollider->gimmickAttrs.push_back(GimmickTriangleAttribute{
                GimmickTriangleAttribute::kind_t::Wall
            });
        }
    }

    ModelBuffer buildRoadModel(const CourseSegment& segment, CoursePolygoneCollider* outCollider)
    {
        Array<ModelVertex> groundVertices((segment.midwayStrips.size() - 1) * 8);
        Array<uint16_t> groundIndices((segment.midwayStrips.size() - 1) * 12);
        int groundVertexOffset{};
        int groundIndexOffset{};

        const bool hasBarrier = segment.style == CourseSegmentStyle::BarrierRoad;

        Array<ModelVertex> gimmickVertices(hasBarrier ? (segment.midwayStrips.size() - 1) * 2 * 8 : 0);
        Array<uint16_t> gimmickIndices(hasBarrier ? (segment.midwayStrips.size() - 1) * 2 * 12 : 0);
        int gimmickVertexOffset{};
        int gimmickIndexOffset{};

        for (int m = 0; m < segment.midwayStrips.size() - 1; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            const FaceVertex l0{s0.leftmost, s0.normal};
            const FaceVertex r0{s0.rightmost, s0.normal};
            const FaceVertex l1{s1.leftmost, s1.normal};
            const FaceVertex r1{s1.rightmost, s1.normal};

            pushGroundFaces(
                groundVertices, groundIndices, groundVertexOffset, groundIndexOffset,
                l0, r0, l1, r1,
                outCollider);

            if (hasBarrier)
            {
                constexpr float barrierHeight = 1.5;

                const Float3 s0_l2r = (s0.rightmost - s0.leftmost).normalized();

                const FaceVertex l0b{s0.leftmost, s0_l2r};
                const FaceVertex l1b{s1.leftmost, s0_l2r};

                const FaceVertex l0t{s0.leftmost + s0.normal * barrierHeight, s0_l2r};
                const FaceVertex l1t{s1.leftmost + s0.normal * barrierHeight, s0_l2r};

                const FaceVertex r0b{s0.rightmost, -s0_l2r};
                const FaceVertex r1b{s1.rightmost, -s0_l2r};

                const FaceVertex r0t{s0.rightmost + s0.normal * barrierHeight, -s0_l2r};
                const FaceVertex r1t{s1.rightmost + s0.normal * barrierHeight, -s0_l2r};

                pushBarrierFaces(
                    gimmickVertices, gimmickIndices, gimmickVertexOffset, gimmickIndexOffset,
                    l0b, l1b, l0t, l1t,
                    outCollider);

                pushBarrierFaces(
                    gimmickVertices, gimmickIndices, gimmickVertexOffset, gimmickIndexOffset,
                    r0b, r1b, r0t, r1t,
                    outCollider);
            }
        }

        Array<ModelMaterial> materials{};
        materials.push_back({
            .name = "plain",
            .parameters = {
                .diffuse = Float3::One() * 0.5f
            }
        });
        Array<ModelShape> shapes{
            ModelShape{std::move(groundVertices), std::move(groundIndices), 0}
        };

        if (hasBarrier)
        {
            materials.push_back({
                .name = "barrier",
                .parameters = {
                    .diffuse = Float3::One() * 1.0f
                }
            });
            shapes.push_back(ModelShape{std::move(gimmickVertices), std::move(gimmickIndices), 1});
        }

        ModelBuffer modelBuffer{
            ModelShapeBuffer{std::move(shapes)}, std::move(materials)
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
        int ground_v{};
        int ground_i{};

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
                    const float s1_rate = static_cast<float>(s + 1) / PipeEntryExitStrips;
                    FaceVertex l0, r0, l1, r1;
                    l0.pos = cap_l0.pos * (1 - s0_rate) + cap_l1.pos * s0_rate;
                    r0.pos = cap_r0.pos * (1 - s0_rate) + cap_r1.pos * s0_rate;
                    l1.pos = cap_l0.pos * (1 - s1_rate) + cap_l1.pos * s1_rate;
                    r1.pos = cap_r0.pos * (1 - s1_rate) + cap_r1.pos * s1_rate;
                    l0.normal = (cap_l0.normal * (1 - s0_rate) + cap_l1.normal * s0_rate).normalized();
                    r0.normal = (cap_r0.normal * (1 - s0_rate) + cap_r1.normal * s0_rate).normalized();
                    l1.normal = (cap_l0.normal * (1 - s1_rate) + cap_l1.normal * s1_rate).normalized();
                    r1.normal = (cap_r0.normal * (1 - s1_rate) + cap_r1.normal * s1_rate).normalized();

                    pushGroundFaces(
                        vertices, indices, ground_v, ground_i,
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

                pushGroundFaces(
                    vertices, indices, ground_v, ground_i,
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
                    const float s1_rate = static_cast<float>(s + 1) / PipeEntryExitStrips;
                    FaceVertex l0, r0, l1, r1;
                    l0.pos = cap_l0.pos * (1 - s0_rate) + cap_l1.pos * s0_rate;
                    r0.pos = cap_r0.pos * (1 - s0_rate) + cap_r1.pos * s0_rate;
                    l1.pos = cap_l0.pos * (1 - s1_rate) + cap_l1.pos * s1_rate;
                    r1.pos = cap_r0.pos * (1 - s1_rate) + cap_r1.pos * s1_rate;
                    l0.normal = (cap_l0.normal * (1 - s0_rate) + cap_l1.normal * s0_rate).normalized();
                    r0.normal = (cap_r0.normal * (1 - s0_rate) + cap_r1.normal * s0_rate).normalized();
                    l1.normal = (cap_l0.normal * (1 - s1_rate) + cap_l1.normal * s1_rate).normalized();
                    r1.normal = (cap_r0.normal * (1 - s1_rate) + cap_r1.normal * s1_rate).normalized();

                    pushGroundFaces(
                        vertices, indices, ground_v, ground_i,
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

        if (segment.style == CourseSegmentStyle::Road ||
            segment.style == CourseSegmentStyle::BarrierRoad)
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
