#include "pch.h"
#include "CourseModelBuilder.h"

#include "CourseConstants.h"
#include "RaceSharedState.h"
#include "TY/DynamicTexture.h"
#include "TY/Image.h"
#include "TY/Quaternion.h"
#include "TY/Immediate3D.h"
#include "TY/InlineComponent.h"
#include "TY/Palette.h"
#include "TY/Rect.h"

using namespace Race;

namespace
{
    Image createStartingLineImage()
    {
        constexpr int half = 32;
        Image image{Size{half * 2, half * 2}};
        for (int y = 0; y < image.size().x; ++y)
        {
            for (int x = 0; x < image.size().y; ++x)
            {
                const bool isWhite = (x / half + y / half) % 2 == 0;
                image[{x, y}] = (isWhite ? Palette::White : Palette::Black).toColorU8();
            }
        }

        return image;
    }

    struct BuilderCache : IInlineComponent
    {
        DynamicTexture startingLineTexture{createStartingLineImage()};
    };

    InlineComponent<BuilderCache> s_builderCache{};

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
        const CourseModelBuilderOptions& options,
        const RectF& uvRect = RectF{0, 0, 1, 1})
    {
        vertices[v_offset] = ModelVertex{r1.pos, r1.normal, uvRect.bl()};
        vertices[v_offset + 1] = ModelVertex{l1.pos, l1.normal, uvRect.br()};
        vertices[v_offset + 2] = ModelVertex{r0.pos, r0.normal, uvRect.tl()};
        vertices[v_offset + 3] = ModelVertex{l0.pos, l0.normal, uvRect.tr()};

        indices[i_offset] = v_offset;
        indices[i_offset + 1] = v_offset + 2;
        indices[i_offset + 2] = v_offset + 1;
        indices[i_offset + 3] = v_offset + 1;
        indices[i_offset + 4] = v_offset + 2;
        indices[i_offset + 5] = v_offset + 3;

        v_offset += 4;
        i_offset += 6;

        vertices[v_offset] = ModelVertex{r1.pos, -r1.normal, uvRect.bl()};
        vertices[v_offset + 1] = ModelVertex{l1.pos, -l1.normal, uvRect.br()};
        vertices[v_offset + 2] = ModelVertex{r0.pos, -r0.normal, uvRect.tl()};
        vertices[v_offset + 3] = ModelVertex{l0.pos, -l0.normal, uvRect.tr()};

        indices[i_offset] = v_offset;
        indices[i_offset + 1] = v_offset + 1;
        indices[i_offset + 2] = v_offset + 2;
        indices[i_offset + 3] = v_offset + 1;
        indices[i_offset + 4] = v_offset + 3;
        indices[i_offset + 5] = v_offset + 2;

        v_offset += 4;
        i_offset += 6;

        if (options.outCollider)
        {
            const std::array normals_00_10_01_11{
                /* 00: */ r0.normal, /* 10: */ l0.normal, /* 01: */ r1.normal, /* 11: */ l1.normal
            };

            // 11 +-----+ 01
            //    |\    |
            //    | \   |
            //    |  C  |
            //    | D \ |
            //    |    \|
            // 10 +-----+ 00

            const Float3& p00 = r0.pos;
            const Float3& p10 = l0.pos;
            const Float3& p01 = r1.pos;
            const Float3& p11 = l1.pos;

            const Float3 C = (p10 + p01) * 0.5f;
            const Float3 D = (p00 + p10 + p01 + p11) * 0.25f;
            const Float3 CD = D - C;

            const Float3 N = r0.normal + l0.normal + r1.normal + l1.normal;

            // 双曲面が二つの三角形の上側に張るようにする
            if (CD.dot(N) >= 0)
            {
                // 10-01 対角線
                options.outCollider->groundTris.push_back(IndexedTriangle{
                    p10, p01, p00, options.outCollider->groundAttrs.size()
                });
                options.outCollider->groundAttrs.push_back(GroundTriangleAttribute{
                    GroundTriangleAttribute::Triangle_10_01_00,
                    normals_00_10_01_11,
                    p11
                });

                options.outCollider->groundTris.push_back(IndexedTriangle{
                    p10, p11, p01, options.outCollider->groundAttrs.size()
                });
                options.outCollider->groundAttrs.push_back(GroundTriangleAttribute{
                    GroundTriangleAttribute::Triangle_10_11_01,
                    normals_00_10_01_11,
                    p00
                });
            }
            else
            {
                // 00-11 対角線
                options.outCollider->groundTris.push_back(IndexedTriangle{
                    p00, p10, p11, options.outCollider->groundAttrs.size()
                });
                options.outCollider->groundAttrs.push_back(GroundTriangleAttribute{
                    GroundTriangleAttribute::Triangle_00_10_11,
                    normals_00_10_01_11,
                    p01
                });

                options.outCollider->groundTris.push_back(IndexedTriangle{
                    p00, p11, p01, options.outCollider->groundAttrs.size()
                });
                options.outCollider->groundAttrs.push_back(GroundTriangleAttribute{
                    GroundTriangleAttribute::Triangle_00_11_01,
                    normals_00_10_01_11,
                    p10
                });
            }
        }
    }

    void pushGimmickFaces(
        Array<ModelVertex>& vertices,
        Array<uint16_t>& indices,
        int& v_offset,
        int& i_offset,
        int stripIndex,
        const FaceVertex& l0,
        const FaceVertex& r0,
        const FaceVertex& l1,
        const FaceVertex& r1,
        GimmickTriangleAttribute::kind_t gimmick,
        const CourseModelBuilderOptions& options,
        const RectF& uvRect = RectF{0, 0, 1, 1})
    {
        vertices[v_offset] = ModelVertex{r1.pos, r1.normal, uvRect.bl()};
        vertices[v_offset + 1] = ModelVertex{l1.pos, l1.normal, uvRect.br()};
        vertices[v_offset + 2] = ModelVertex{r0.pos, r0.normal, uvRect.tl()};
        vertices[v_offset + 3] = ModelVertex{l0.pos, l0.normal, uvRect.tr()};

        indices[i_offset] = v_offset;
        indices[i_offset + 1] = v_offset + 2;
        indices[i_offset + 2] = v_offset + 1;
        indices[i_offset + 3] = v_offset + 1;
        indices[i_offset + 4] = v_offset + 2;
        indices[i_offset + 5] = v_offset + 3;

        v_offset += 4;
        i_offset += 6;

        vertices[v_offset] = ModelVertex{r1.pos, -r1.normal, uvRect.bl()};
        vertices[v_offset + 1] = ModelVertex{l1.pos, -l1.normal, uvRect.br()};
        vertices[v_offset + 2] = ModelVertex{r0.pos, -r0.normal, uvRect.tl()};
        vertices[v_offset + 3] = ModelVertex{l0.pos, -l0.normal, uvRect.tr()};

        indices[i_offset] = v_offset;
        indices[i_offset + 1] = v_offset + 1;
        indices[i_offset + 2] = v_offset + 2;
        indices[i_offset + 3] = v_offset + 1;
        indices[i_offset + 4] = v_offset + 3;
        indices[i_offset + 5] = v_offset + 2;

        v_offset += 4;
        i_offset += 6;

        if (options.outCollider)
        {
            options.outCollider->gimmickTris.push_back(IndexedTriangle{
                r1.pos, r0.pos, l1.pos, options.outCollider->gimmickAttrs.size()
            });
            options.outCollider->gimmickAttrs.push_back(GimmickTriangleAttribute{
                gimmick
            });

            options.outCollider->gimmickTris.push_back(IndexedTriangle{
                l1.pos, r0.pos, l0.pos, options.outCollider->gimmickAttrs.size()
            });
            options.outCollider->gimmickAttrs.push_back(GimmickTriangleAttribute{
                gimmick
            });
        }

        if (options.outGimmickPlacements)
        {
            options.outGimmickPlacements->push_back(GimmickPlacement{
                .kind = GimmickTriangleAttribute{gimmick},
                .stripIndex = stripIndex,
                .left = (l0.pos + l1.pos) * 0.5f,
                .right = (r0.pos + r1.pos) * 0.5f,
            });
        }
    }

    void buildRoadModel(
        ModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        const bool createStartingLine = options.createStartingLine;
        constexpr int startingLineStripCount = 2;

        {
            Array<ModelVertex> vertices((segment.midwayStrips.size() - 1) * 8);
            Array<uint16_t> indices((segment.midwayStrips.size() - 1) * 12);
            int v_offset{};
            int i_offset{};

            const int m0 = createStartingLine ? startingLineStripCount : 0;
            for (int m = m0; m < segment.midwayStrips.size() - 1; ++m)
            {
                auto& s0 = segment.midwayStrips[m];
                auto& s1 = segment.midwayStrips[m + 1];

                const FaceVertex l0{s0.leftmost, s0.normal};
                const FaceVertex r0{s0.rightmost, s0.normal};
                const FaceVertex l1{s1.leftmost, s1.normal};
                const FaceVertex r1{s1.rightmost, s1.normal};

                pushGroundFaces(
                    vertices, indices, v_offset, i_offset,
                    l0, r0, l1, r1,
                    options);
            }

            model.shapes.push_back(ModelShape{
                std::move(vertices), std::move(indices), static_cast<uint16_t>(model.materials.size())
            });
            model.materials.push_back({
                .name = "plain",
                .parameters = {
                    .diffuse = sRGB(Float3::One() * 0.5f).toFloat3()
                }
            });
        }

        if (createStartingLine)
        {
            Array<ModelVertex> vertices(startingLineStripCount * 8);
            Array<uint16_t> indices(startingLineStripCount * 12);
            int v_offset{};
            int i_offset{};

            constexpr float texH = 1.0f / startingLineStripCount;
            float texW{};
            for (int m = 0; m < startingLineStripCount; ++m)
            {
                auto& s0 = segment.midwayStrips[m];
                auto& s1 = segment.midwayStrips[m + 1];

                const FaceVertex l0{s0.leftmost, s0.normal};
                const FaceVertex r0{s0.rightmost, s0.normal};
                const FaceVertex l1{s1.leftmost, s1.normal};
                const FaceVertex r1{s1.rightmost, s1.normal};

                if (m == 0)
                {
                    assert((s1.center - s0.center).length()>0);
                    texW = texH * (s0.rightmost - s0.leftmost).length() / (s1.center - s0.center).length();
                }

                pushGroundFaces(
                    vertices, indices, v_offset, i_offset,
                    l0, r0, l1, r1,
                    options, RectF{0.0f, texH * m, texW, texH});
            }

            model.shapes.push_back(ModelShape{
                std::move(vertices), std::move(indices), static_cast<uint16_t>(model.materials.size())
            });
            model.materials.push_back({
                .name = "starting_line",
                .parameters = {
                    .diffuse = Float3::One(),
                },
                .diffuseTexture = s_builderCache->startingLineTexture,
            });
        }
    }

    void buildPipeModel(
        ModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        // TODO: 終端部分の調整

        constexpr int subdivision = PipeSubdivision;
        constexpr int halfSubdivision0 = subdivision / 2;
        constexpr int halfSubdivision1 = halfSubdivision0 + 1;

        const int hasEntry = segment.midwayStrips.size() > PipeEntryExitStrips &&
            segment.midwayStrips[0].style != CourseSegmentStyle::Pipe;

        const bool hasExit = segment.midwayStrips.size() > PipeEntryExitStrips &&
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

        constexpr float r = PipeRadius;

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
                        vertices, indices, v_offset, i_offset,
                        l0, r0, l1, r1,
                        options);
                }
            }
        }

        for (int m = hasEntry * PipeEntryExitStrips; m < hasEntry * PipeEntryExitStrips + pipeStrips - 1; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            std::array<Float3, subdivision> n0s = s0.pipe.ringVectors;
            std::array<Float3, subdivision> n1s = s1.pipe.ringVectors;

            // 円周上の面作成
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
                    vertices, indices, v_offset, i_offset,
                    l0, r0, l1, r1,
                    options);
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
                        vertices, indices, v_offset, i_offset,
                        l0, r0, l1, r1,
                        options);
                }
            }
        }

        model.shapes.push_back(
            ModelShape{std::move(vertices), std::move(indices), static_cast<uint16_t>(model.materials.size())}
        );
        model.materials.push_back({
            .name = "plain",
            .parameters = {
                .diffuse = sRGB(Float3::One() * 0.5f).toFloat3()
            }
        });
    }

    void buildCylinderModel(ModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        constexpr int subdivision = CylinderSubdivision;
        constexpr int entryExitSubdivision = CylinderSubdivision * 2;

        const int hasEntry = segment.midwayStrips.size() > CylinderEntryExitStrips &&
            segment.midwayStrips[0].style != CourseSegmentStyle::Cylinder;

        const bool hasExit = segment.midwayStrips.size() > CylinderEntryExitStrips &&
            segment.midwayStrips[segment.midwayStrips.size() - 1].style != CourseSegmentStyle::Cylinder;

        constexpr int innerEntryExitStrips = CylinderEntryExitStrips / 2;
        const int cylinderStrips = segment.midwayStrips.size() - (hasEntry + hasExit) * innerEntryExitStrips;

        // -----------------------------------------------

        Array<ModelVertex> vertices(
            (hasEntry + hasExit) * CylinderEntryExitStrips * (entryExitSubdivision * 4 * 2) +
            (cylinderStrips - 1) * (subdivision * 4 * 2));
        Array<uint16_t> indices(
            (hasEntry + hasExit) * CylinderEntryExitStrips * (entryExitSubdivision * 6 * 2) +
            (cylinderStrips - 1) * (subdivision * 6 * 2));
        int v_offset{};
        int i_offset{};

        // -----------------------------------------------

        constexpr float baseRadius = CylinderRadius;

        constexpr float outerEntryExitRadius = baseRadius * 3.0f;

        if (hasEntry)
        {
            auto& s0 = segment.midwayStrips[0];
            assert(s0.style != CourseSegmentStyle::Cylinder);

            auto& s1 = segment.midwayStrips[CylinderEntryExitStrips];
            assert(s1.style == CourseSegmentStyle::Cylinder);

            const Float3 n = s0.normal;
            const Float3 axis = (s1.center - s0.center).normalized();

            for (int i0 = 0; i0 < entryExitSubdivision - 1; ++i0)
            {
                const int i1 = i0 + 1;
                const float t0 = static_cast<float>(i0) / (entryExitSubdivision - 1);
                const float t1 = static_cast<float>(i1) / (entryExitSubdivision - 1);

                FaceVertex cap_l0, cap_r0, cap_l1, cap_r1;

                cap_l0.pos = s0.leftmost * (1 - t0) + s0.rightmost * t0;
                cap_r0.pos = s0.leftmost * (1 - t1) + s0.rightmost * t1;
                cap_l0.normal = s0.normal;
                cap_r0.normal = s0.normal;

                {
                    const float angle0 = Math::HalfPiF + t0 * Math::Pi_v<float>;
                    const float angle1 = Math::HalfPiF + t1 * Math::Pi_v<float>;
                    const Float3 v0 = Quaternion(axis, angle0).rotate(n).normalized();
                    const Float3 v1 = Quaternion(axis, angle1).rotate(n).normalized();

                    // 中央部分の勾配を緩やかにする係数
                    // const float smoothness0 = 0.5f + Math::Square(t0 - 0.5f);
                    // const float smoothness1 = 0.5f + Math::Square(t1 - 0.5f);

                    cap_l1.pos = s1.center + v0 * outerEntryExitRadius; // * smoothness0;
                    cap_r1.pos = s1.center + v1 * outerEntryExitRadius; // * smoothness1;
                    cap_l1.normal = -v0;
                    cap_r1.normal = -v1;
                }

                for (int s = 0; s < CylinderEntryExitStrips; ++s)
                {
                    const float s0_rate = static_cast<float>(s) / CylinderEntryExitStrips;
                    const float s1_rate = static_cast<float>(s + 1) / CylinderEntryExitStrips;
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
                        vertices, indices, v_offset, i_offset,
                        l0, r0, l1, r1,
                        options);
                }
            }
        }

        for (int m = hasEntry * innerEntryExitStrips; m < hasEntry * innerEntryExitStrips + cylinderStrips - 1; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            std::array<Float3, subdivision> n0s = s0.pipe.ringVectors;
            std::array<Float3, subdivision> n1s = s1.pipe.ringVectors;

            const auto getRadius = [&](int m_) -> float
            {
                if (hasEntry)
                {
                    m_ -= innerEntryExitStrips;
                }

                float radius = baseRadius;
                if (hasEntry && m_ < innerEntryExitStrips)
                {
                    radius *= ( // std::sqrtf(
                        1.0f - Math::Square(1.0f - static_cast<float>(m_) / innerEntryExitStrips));
                }
                else if (hasExit && m_ >= cylinderStrips - 1 - innerEntryExitStrips)
                {
                    radius *= ( // std::sqrtf(
                        1.0f - Math::Square(1.0f - static_cast<float>(cylinderStrips - 1 - m_) / innerEntryExitStrips));
                }

                return radius;
            };

            const float radius0 = getRadius(m);
            const float radius1 = getRadius(m + 1);

            // 円周上の面作成
            for (int i0 = 0; i0 < subdivision; ++i0)
            {
                const int i1 = (i0 + 1) % subdivision;

                FaceVertex l0, r0, l1, r1;

                r0.pos = s0.center + n0s[i0] * radius0;
                l0.pos = s0.center + n0s[i1] * radius0;
                r1.pos = s1.center + n1s[i0] * radius1;
                l1.pos = s1.center + n1s[i1] * radius1;

                r0.normal = n0s[i0];
                l0.normal = n0s[i1];
                r1.normal = n1s[i0];
                l1.normal = n1s[i1];

                pushGroundFaces(
                    vertices, indices, v_offset, i_offset,
                    l0, r0, l1, r1,
                    options);
            }
        }

        if (hasExit)
        {
            auto& s0 = segment.midwayStrips[segment.midwayStrips.size() - 1 - CylinderEntryExitStrips];
            assert(s0.style == CourseSegmentStyle::Cylinder);

            auto& s1 = segment.midwayStrips[segment.midwayStrips.size() - 1];
            assert(s1.style != CourseSegmentStyle::Cylinder);

            const Float3 n = s1.normal;
            const Float3 axis = (s1.center - s0.center).normalized();

            for (int i0 = 0; i0 < entryExitSubdivision - 1; ++i0)
            {
                const int i1 = i0 + 1;
                const float t0 = static_cast<float>(i0) / (entryExitSubdivision - 1);
                const float t1 = static_cast<float>(i1) / (entryExitSubdivision - 1);

                FaceVertex cap_l0, cap_r0, cap_l1, cap_r1;

                {
                    const float angle0 = Math::HalfPiF + t0 * Math::Pi_v<float>;
                    const float angle1 = Math::HalfPiF + t1 * Math::Pi_v<float>;
                    const Float3 v0 = Quaternion(axis, angle0).rotate(n).normalized();
                    const Float3 v1 = Quaternion(axis, angle1).rotate(n).normalized();

                    // 中央部分の勾配を緩やかにする係数
                    // const float smoothness0 = 0.5f + Math::Square(t0 - 0.5f);
                    // const float smoothness1 = 0.5f + Math::Square(t1 - 0.5f);

                    cap_l0.pos = s0.center + v0 * outerEntryExitRadius; // * smoothness0;
                    cap_r0.pos = s0.center + v1 * outerEntryExitRadius; // * smoothness1;
                    cap_l0.normal = -v0;
                    cap_r0.normal = -v1;
                }

                cap_l1.pos = s1.leftmost * (1 - t0) + s1.rightmost * t0;
                cap_r1.pos = s1.leftmost * (1 - t1) + s1.rightmost * t1;
                cap_l1.normal = s1.normal;
                cap_r1.normal = s1.normal;

                for (int s = 0; s < CylinderEntryExitStrips; ++s)
                {
                    const float s0_rate = static_cast<float>(s) / CylinderEntryExitStrips;
                    const float s1_rate = static_cast<float>(s + 1) / CylinderEntryExitStrips;
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
                        vertices, indices, v_offset, i_offset,
                        l0, r0, l1, r1,
                        options);
                }
            }
        }

        model.shapes.push_back(
            ModelShape{std::move(vertices), std::move(indices), static_cast<uint16_t>(model.materials.size())}
        );
        model.materials.push_back({
            .name = "plain",
            .parameters = {
                .diffuse = sRGB(Float3::One() * 0.5f).toFloat3()
            }
        });
    }

    // -----------------------------------------------

    void buildBarrier_Road(ModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        Array<ModelVertex> vertices((segment.midwayStrips.size() - 1) * 2 * 8);
        Array<uint16_t> indices((segment.midwayStrips.size() - 1) * 2 * 12);
        int v_offset{};
        int i_offset{};

        for (int m = 0; m < segment.midwayStrips.size() - 1; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            constexpr float barrierHeight = 2.5f;

            const Float3 s0_l2r = (s0.rightmost - s0.leftmost).normalized();
            const Float3 s1_l2r = (s1.rightmost - s1.leftmost).normalized();

            const FaceVertex l0b{s0.leftmost, s0_l2r};
            const FaceVertex l1b{s1.leftmost, s1_l2r};

            const FaceVertex l0t{s0.leftmost + s0.normal * barrierHeight, s0_l2r};
            const FaceVertex l1t{s1.leftmost + s1.normal * barrierHeight, s1_l2r};

            const FaceVertex r0b{s0.rightmost, -s0_l2r};
            const FaceVertex r1b{s1.rightmost, -s1_l2r};

            const FaceVertex r0t{s0.rightmost + s0.normal * barrierHeight, -s0_l2r};
            const FaceVertex r1t{s1.rightmost + s1.normal * barrierHeight, -s1_l2r};

            pushGimmickFaces(
                vertices, indices, v_offset, i_offset,
                m, l0b, l1b, l0t, l1t,
                GimmickTriangleAttribute::kind_t::Barrier,
                options);
            pushGimmickFaces(
                vertices, indices, v_offset, i_offset,
                m, r1b, r0b, r1t, r0t,
                GimmickTriangleAttribute::kind_t::Barrier,
                options);
        }

        model.shapes.push_back(ModelShape{
            std::move(vertices), std::move(indices), static_cast<uint16_t>(model.materials.size())
        });
        model.materials.push_back({
            .name = "barrier",
            .parameters = {
                .diffuse = sRGB(0.97f, 0.53f, 0.00f).toFloat3()
            }
        });
    }

    enum class LCR : uint8_t
    {
        L,
        C,
        R,
    };

    int getCircularFaceIndex(LCR lcr)
    {
        switch (lcr)
        {
        case LCR::L:
            return 4;
        case LCR::C:
            return 0;
        case LCR::R:
            return 2;
        default:
            assert(false);
            return 0;
        }
    }

    void buildPad_Road(
        ModelData& model,
        const CourseSegment& segment,
        LCR lcr,
        GimmickTriangleAttribute::kind_t gimmick,
        const CourseModelBuilderOptions& options)
    {
        constexpr float padElevation = 0.5f;
        constexpr float padLength = 10.0f;

        const int s0_index = segment.midwayStrips.size() / 2 - 1;
        if (not InRange<int>(s0_index, 0, segment.midwayStrips.size() - 2))
        {
            return;
        }

        auto& s0 = segment.midwayStrips[s0_index];
        auto& s1 = segment.midwayStrips[s0_index + 1];

        const float padWidth = (s0.rightmost - s0.leftmost).length() / 3.0f;

        const Float3 normal = (s0.normal + s1.normal).normalized();
        const Float3 toRight = ((s0.rightmost - s0.leftmost) + (s1.rightmost - s1.leftmost)).normalized();
        const Float3 toForward = toRight.cross(normal).normalized();

        float laneOffset{};
        switch (lcr)
        {
        case LCR::L:
            laneOffset = -padWidth;
            break;
        case LCR::C:
            break;
        case LCR::R:
            laneOffset = padWidth;
            break;
        default:
            assert(false);
            return;
        }

        const Float3 center = (s0.center + s1.center) * 0.5f
            + (s0.normal + s1.normal) * 0.5f * padElevation
            + toRight * laneOffset;

        const FaceVertex l0{
            center - toRight * (padWidth * 0.5f) - toForward * (padLength * 0.5f),
            normal
        };
        const FaceVertex r0{
            center + toRight * (padWidth * 0.5f) - toForward * (padLength * 0.5f),
            normal
        };
        const FaceVertex l1{
            center - toRight * (padWidth * 0.5f) + toForward * (padLength * 0.5f),
            normal
        };
        const FaceVertex r1{
            center + toRight * (padWidth * 0.5f) + toForward * (padLength * 0.5f),
            normal
        };

        Array<ModelVertex> vertices(8);
        Array<uint16_t> indices(12);
        int v_offset{};
        int i_offset{};

        pushGimmickFaces(
            vertices, indices, v_offset, i_offset,
            s0_index, l0, r0, l1, r1,
            gimmick,
            options);

        model.shapes.push_back(ModelShape{
            std::move(vertices), std::move(indices), static_cast<uint16_t>(model.materials.size())
        });

        if (gimmick == GimmickTriangleAttribute::kind_t::BoostPad)
        {
            model.materials.push_back({
                .name = "boost_pad",
                .parameters = {
                    .diffuse = Float3::One()
                },
                .diffuseTexture = g_sharedState->gimmickTextures.boostPad.getFrontRtv()
            });
        }
        else
        {
            assert(gimmick == GimmickTriangleAttribute::kind_t::JumpPad);
            model.materials.push_back({
                .name = "jump_pad",
                .parameters = {
                    .diffuse = Float3::One()
                },
                .diffuseTexture = g_sharedState->gimmickTextures.jumpPad.getFrontRtv()
            });
        }
    }

    void buildPad_Circular(
        ModelData& model,
        const CourseSegment& segment,
        LCR lcr,
        GimmickTriangleAttribute::kind_t gimmick,
        const CourseModelBuilderOptions& options)
    {
        constexpr float padElevation = 0.5f;
        constexpr float padLength = 10.0f;
        static_assert(PipeSubdivision == CylinderSubdivision);

        const int s0_index = segment.midwayStrips.size() / 2 - 1;
        if (not InRange<int>(s0_index, 0, segment.midwayStrips.size() - 2))
        {
            return;
        }

        const auto& s0 = segment.midwayStrips[s0_index];
        const auto& s1 = segment.midwayStrips[s0_index + 1];

        const int faceIndex0 = getCircularFaceIndex(lcr);
        const int faceIndex1 = (faceIndex0 + 1) % PipeSubdivision;
        const float radius = segment.style == CourseSegmentStyle::Pipe
                                 ? PipeRadius
                                 : CylinderRadius;

        const auto createFaceVertices = [&](const CourseStrip& strip)
            -> std::pair<FaceVertex, FaceVertex>
        {
            const Float3& n0 = strip.pipe.ringVectors[faceIndex0];
            const Float3& n1 = strip.pipe.ringVectors[faceIndex1];

            if (segment.style == CourseSegmentStyle::Pipe)
            {
                return {
                    FaceVertex{strip.center + n0 * radius, -n0},
                    FaceVertex{strip.center + n1 * radius, -n1}
                };
            }
            else // Cylinder
            {
                return {
                    FaceVertex{strip.center + n1 * radius, n1},
                    FaceVertex{strip.center + n0 * radius, n0}
                };
            }
        };

        const auto [surfaceL0, surfaceR0] = createFaceVertices(s0);
        const auto [surfaceL1, surfaceR1] = createFaceVertices(s1);

        const Float3 normal =
            (surfaceL0.normal + surfaceR0.normal + surfaceL1.normal + surfaceR1.normal).normalized();
        const Float3 toRight =
            ((surfaceR0.pos - surfaceL0.pos) + (surfaceR1.pos - surfaceL1.pos)).normalized();
        const Float3 toForward = toRight.cross(normal).normalized();
        const float padWidth =
            ((surfaceR0.pos - surfaceL0.pos).length() + (surfaceR1.pos - surfaceL1.pos).length()) * 0.5f;
        const Float3 center =
            (surfaceL0.pos + surfaceR0.pos + surfaceL1.pos + surfaceR1.pos) * 0.25f
            + normal * padElevation;

        const FaceVertex l0{
            center - toRight * (padWidth * 0.5f) - toForward * (padLength * 0.5f),
            normal
        };
        const FaceVertex r0{
            center + toRight * (padWidth * 0.5f) - toForward * (padLength * 0.5f),
            normal
        };
        const FaceVertex l1{
            center - toRight * (padWidth * 0.5f) + toForward * (padLength * 0.5f),
            normal
        };
        const FaceVertex r1{
            center + toRight * (padWidth * 0.5f) + toForward * (padLength * 0.5f),
            normal
        };

        Array<ModelVertex> vertices(8);
        Array<uint16_t> indices(12);
        int v_offset{};
        int i_offset{};

        pushGimmickFaces(
            vertices, indices, v_offset, i_offset,
            s0_index, l0, r0, l1, r1,
            gimmick,
            options);

        model.shapes.push_back(ModelShape{
            std::move(vertices), std::move(indices), static_cast<uint16_t>(model.materials.size())
        });

        if (gimmick == GimmickTriangleAttribute::kind_t::BoostPad)
        {
            model.materials.push_back({
                .name = "boost_pad",
                .parameters = {
                    .diffuse = Float3::One()
                },
                .diffuseTexture = g_sharedState->gimmickTextures.boostPad.getFrontRtv()
            });
        }
        else
        {
            assert(gimmick == GimmickTriangleAttribute::kind_t::JumpPad);
            model.materials.push_back({
                .name = "jump_pad",
                .parameters = {
                    .diffuse = Float3::One()
                },
                .diffuseTexture = g_sharedState->gimmickTextures.jumpPad.getFrontRtv()
            });
        }
    }

    std::pair<Float3, Float3> separateStrip(const CourseStrip& s, LCR lcr)
    {
        switch (lcr)
        {
        case LCR::L:
            return {
                s.leftmost,
                Math::Lerp3D(s.leftmost, s.center, 2.0f / 3.0f)
            };
        case LCR::C:
            return {
                Math::Lerp3D(s.leftmost, s.center, 2.0f / 3.0f),
                Math::Lerp3D(s.center, s.rightmost, 1.0f / 3.0f)
            };
        case LCR::R:
            return {
                Math::Lerp3D(s.center, s.rightmost, 1.0f / 3.0f),
                s.rightmost
            };
        default:
            assert(false);
            return {};
        }
    }

    void buildPitZone_Road(ModelData& model, const CourseSegment& segment, LCR lcr,
                           const CourseModelBuilderOptions& options)
    {
        constexpr float padElevation = 0.5f;

        const int s0_index = segment.midwayStrips.size() / 2 - 1;
        if (not InRange<int>(s0_index, 0, segment.midwayStrips.size() - 2))
        {
            return;
        }

        Array<ModelVertex> vertices((segment.midwayStrips.size() - 1) * 8);
        Array<uint16_t> indices((segment.midwayStrips.size() - 1) * 12);
        int v_offset{};
        int i_offset{};

        float texY{};
        for (int m = 0; m < segment.midwayStrips.size() - 1; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            const auto lr0 = separateStrip(s0, lcr);
            const auto lr1 = separateStrip(s1, lcr);

            const FaceVertex l0{lr0.first + s0.normal * padElevation, s0.normal};
            const FaceVertex r0{lr0.second + s0.normal * padElevation, s0.normal};
            const FaceVertex l1{lr1.first + s1.normal * padElevation, s1.normal};
            const FaceVertex r1{lr1.second + s1.normal * padElevation, s1.normal};

            const float texH = 2.0f * (s1.center - s0.center).length() / (s0.rightmost - s0.leftmost).length();

            pushGimmickFaces(
                vertices, indices, v_offset, i_offset,
                m, l0, r0, l1, r1,
                GimmickTriangleAttribute::kind_t::PitZone,
                options,
                RectF{0.0f, texY, 1.0f, texH});

            texY += texH;
        }

        model.shapes.push_back(ModelShape{
            std::move(vertices), std::move(indices), static_cast<uint16_t>(model.materials.size())
        });
        model.materials.push_back({
            .name = "pit_zone",
            .parameters = {
                .diffuse = Float3::One()
            },
            .diffuseTexture = g_sharedState->gimmickTextures.pitZone.getFrontRtv()
        });
    }

    void buildGimmickModel(ModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        for (const auto& gimmick : segment.gimmicks)
        {
            switch (gimmick)
            {
            case CourseGimmickKind::Barrier:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildBarrier_Road(model, segment, options);
                }
                break;
            case CourseGimmickKind::BoostPad_L:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildPad_Road(
                        model, segment, LCR::L, GimmickTriangleAttribute::kind_t::BoostPad, options);
                }
                else if (segment.style == CourseSegmentStyle::Pipe ||
                    segment.style == CourseSegmentStyle::Cylinder)
                {
                    buildPad_Circular(
                        model, segment, LCR::L, GimmickTriangleAttribute::kind_t::BoostPad, options);
                }
                break;
            case CourseGimmickKind::BoostPad_C:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildPad_Road(
                        model, segment, LCR::C, GimmickTriangleAttribute::kind_t::BoostPad, options);
                }
                else if (segment.style == CourseSegmentStyle::Pipe ||
                    segment.style == CourseSegmentStyle::Cylinder)
                {
                    buildPad_Circular(
                        model, segment, LCR::C, GimmickTriangleAttribute::kind_t::BoostPad, options);
                }
                break;
            case CourseGimmickKind::BoostPad_R:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildPad_Road(
                        model, segment, LCR::R, GimmickTriangleAttribute::kind_t::BoostPad, options);
                }
                else if (segment.style == CourseSegmentStyle::Pipe ||
                    segment.style == CourseSegmentStyle::Cylinder)
                {
                    buildPad_Circular(
                        model, segment, LCR::R, GimmickTriangleAttribute::kind_t::BoostPad, options);
                }
                break;
            case CourseGimmickKind::JumpPad_L:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildPad_Road(
                        model, segment, LCR::L, GimmickTriangleAttribute::kind_t::JumpPad, options);
                }
                else if (segment.style == CourseSegmentStyle::Pipe ||
                    segment.style == CourseSegmentStyle::Cylinder)
                {
                    buildPad_Circular(
                        model, segment, LCR::L, GimmickTriangleAttribute::kind_t::JumpPad, options);
                }
                break;
            case CourseGimmickKind::JumpPad_C:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildPad_Road(
                        model, segment, LCR::C, GimmickTriangleAttribute::kind_t::JumpPad, options);
                }
                else if (segment.style == CourseSegmentStyle::Pipe ||
                    segment.style == CourseSegmentStyle::Cylinder)
                {
                    buildPad_Circular(
                        model, segment, LCR::C, GimmickTriangleAttribute::kind_t::JumpPad, options);
                }
                break;
            case CourseGimmickKind::JumpPad_R:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildPad_Road(
                        model, segment, LCR::R, GimmickTriangleAttribute::kind_t::JumpPad, options);
                }
                else if (segment.style == CourseSegmentStyle::Pipe ||
                    segment.style == CourseSegmentStyle::Cylinder)
                {
                    buildPad_Circular(
                        model, segment, LCR::R, GimmickTriangleAttribute::kind_t::JumpPad, options);
                }
                break;
            case CourseGimmickKind::PitZone_L:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildPitZone_Road(model, segment, LCR::L, options);
                }
                break;
            case CourseGimmickKind::PitZone_C:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildPitZone_Road(model, segment, LCR::C, options);
                }
                break;
            case CourseGimmickKind::PitZone_R:
                if (segment.style == CourseSegmentStyle::Road)
                {
                    buildPitZone_Road(model, segment, LCR::R, options);
                }
                break;
            default:
                assert(false && "buildGimmickModel(): gimmick kind is not supported.");
                break;
            }
        }
    }
}

namespace Race
{
    ModelBuffer BuildCourseModel(const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        assert(segment.midwayStrips.size() > 0);

        ModelData model{};

        if (segment.style == CourseSegmentStyle::Road)
        {
            buildRoadModel(model, segment, options);
        }
        else if (segment.style == CourseSegmentStyle::Pipe)
        {
            buildPipeModel(model, segment, options);
        }
        else if (segment.style == CourseSegmentStyle::Cylinder)
        {
            buildCylinderModel(model, segment, options);
        }
        else if (segment.style == CourseSegmentStyle::Gap)
        {
            // Nothing
        }
        else
        {
            assert(false && "BuildCourseModel(): segment.style is not supported.");
            return {};
        }

        buildGimmickModel(model, segment, options);

        return model;
    }
}
