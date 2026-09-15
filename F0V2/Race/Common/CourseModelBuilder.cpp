#include "pch.h"
#include "CourseModelBuilder.h"

#include "CourseConstants.h"
#include "CourseMinimapModelBuilder.h"
#include "GimmickModelBuilder.h"
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

    struct GroundShapeData
    {
        Array<ModelVertex> vertices;
        int vertexOffset{};

        Array<uint16_t> indices;
        int indexOffset{};

        explicit GroundShapeData(int faceCount)
            : vertices(faceCount * 4),
              indices(faceCount * 6)
        {
        }
    };

    void pushGroundTopFace(
        GroundShapeData& shape,
        const FaceVertex& l0,
        const FaceVertex& r0,
        const FaceVertex& l1,
        const FaceVertex& r1,
        const CourseModelBuilderOptions& options,
        const RectF& uvRect = RectF{0, 0, 1, 1})
    {
        shape.vertices[shape.vertexOffset] = ModelVertex{r1.pos, r1.normal, uvRect.bl()};
        shape.vertices[shape.vertexOffset + 1] = ModelVertex{l1.pos, l1.normal, uvRect.br()};
        shape.vertices[shape.vertexOffset + 2] = ModelVertex{r0.pos, r0.normal, uvRect.tl()};
        shape.vertices[shape.vertexOffset + 3] = ModelVertex{l0.pos, l0.normal, uvRect.tr()};

        shape.indices[shape.indexOffset] = shape.vertexOffset;
        shape.indices[shape.indexOffset + 1] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 2] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 3] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 4] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 5] = shape.vertexOffset + 3;

        shape.vertexOffset += 4;
        shape.indexOffset += 6;

        if (options.outMinimapModel)
        {
            options.outMinimapModel->pushGroundQuad(
                {l0.pos, l0.normal}, {r0.pos, r0.normal},
                {l1.pos, l1.normal}, {r1.pos, r1.normal});
        }

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

    void pushGroundBottomFace(
        GroundShapeData& shape,
        const FaceVertex& l0,
        const FaceVertex& r0,
        const FaceVertex& l1,
        const FaceVertex& r1,
        const CourseModelBuilderOptions& options,
        const RectF& uvRect = RectF{0, 0, 1, 1})
    {
        shape.vertices[shape.vertexOffset] = ModelVertex{r1.pos, -r1.normal, uvRect.bl()};
        shape.vertices[shape.vertexOffset + 1] = ModelVertex{l1.pos, -l1.normal, uvRect.br()};
        shape.vertices[shape.vertexOffset + 2] = ModelVertex{r0.pos, -r0.normal, uvRect.tl()};
        shape.vertices[shape.vertexOffset + 3] = ModelVertex{l0.pos, -l0.normal, uvRect.tr()};

        shape.indices[shape.indexOffset] = shape.vertexOffset;
        shape.indices[shape.indexOffset + 1] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 2] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 3] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 4] = shape.vertexOffset + 3;
        shape.indices[shape.indexOffset + 5] = shape.vertexOffset + 2;

        shape.vertexOffset += 4;
        shape.indexOffset += 6;

        // TODO: 様子を見て下面のコライダー追加
    }

    void buildRoadModel(
        ModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        const bool createStartingLine = options.createStartingLine;
        constexpr int startingLineStripCount = 2;

        {
            const int m0 = createStartingLine ? startingLineStripCount : 0;
            const int faceCount = static_cast<int>(segment.midwayStrips.size()) - 1 - m0;
            GroundShapeData topShape{faceCount};
            GroundShapeData bottomShape{faceCount};

            for (int m = m0; m < segment.midwayStrips.size() - 1; ++m)
            {
                auto& s0 = segment.midwayStrips[m];
                auto& s1 = segment.midwayStrips[m + 1];

                const FaceVertex l0{s0.leftmost, s0.normal};
                const FaceVertex r0{s0.rightmost, s0.normal};
                const FaceVertex l1{s1.leftmost, s1.normal};
                const FaceVertex r1{s1.rightmost, s1.normal};

                pushGroundTopFace(
                    topShape,
                    l0, r0, l1, r1,
                    options);
                pushGroundBottomFace(
                    bottomShape,
                    l0, r0, l1, r1,
                    options);
            }

            model.shapes.push_back(ModelShape{
                std::move(topShape.vertices),
                std::move(topShape.indices),
                static_cast<uint16_t>(model.materials.size())
            });
            model.materials.push_back({
                .name = "plain_top",
                .parameters = {
                    .albedo = sRGB(Float3::One() * 0.5f).toFloat3()
                }
            });

            model.shapes.push_back(ModelShape{
                std::move(bottomShape.vertices),
                std::move(bottomShape.indices),
                static_cast<uint16_t>(model.materials.size())
            });
            model.materials.push_back({
                .name = "plain_bottom",
                .parameters = {
                    .albedo = sRGB(Float3::One() * 0.1f).toFloat3()
                }
            });
        }

        if (createStartingLine)
        {
            GroundShapeData topShape{startingLineStripCount};
            GroundShapeData bottomShape{startingLineStripCount};

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

                pushGroundTopFace(
                    topShape,
                    l0, r0, l1, r1,
                    options, RectF{0.0f, texH * m, texW, texH});
                pushGroundBottomFace(
                    bottomShape,
                    l0, r0, l1, r1,
                    options, RectF{0.0f, texH * m, texW, texH});
            }

            model.shapes.push_back(ModelShape{
                std::move(topShape.vertices),
                std::move(topShape.indices),
                static_cast<uint16_t>(model.materials.size())
            });
            model.materials.push_back({
                .name = "starting_line",
                .parameters = {
                    .albedo = Float3::One(),
                },
                .albedoTexture = s_builderCache->startingLineTexture,
            });

            model.shapes.push_back(ModelShape{
                std::move(bottomShape.vertices),
                std::move(bottomShape.indices),
                static_cast<uint16_t>(model.materials.size())
            });
            model.materials.push_back({
                .name = "starting_line_bottom",
                .parameters = {
                    .albedo = sRGB(Float3::One() * 0.1f).toFloat3()
                }
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

        const int faceCount =
            (hasEntry + hasExit) * PipeEntryExitStrips * (halfSubdivision1 - 1) + (pipeStrips - 1) * subdivision;
        GroundShapeData topShape{faceCount};
        GroundShapeData bottomShape{faceCount};

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

                    pushGroundTopFace(
                        topShape,
                        l0, r0, l1, r1,
                        options);
                    pushGroundBottomFace(
                        bottomShape,
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

                pushGroundTopFace(
                    topShape,
                    l0, r0,
                    l1, r1,
                    options);
                pushGroundBottomFace(
                    bottomShape,
                    l0, r0,
                    l1, r1,
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

                    pushGroundTopFace(
                        topShape,
                        l0, r0,
                        l1, r1,
                        options);
                    pushGroundBottomFace(
                        bottomShape,
                        l0, r0,
                        l1, r1,
                        options);
                }
            }
        }

        model.shapes.push_back(ModelShape{
            std::move(topShape.vertices),
            std::move(topShape.indices),
            static_cast<uint16_t>(model.materials.size())
        });
        model.materials.push_back({
            .name = "plain_top",
            .parameters = {
                .albedo = sRGB(Float3::One() * 0.5f).toFloat3()
            }
        });

        model.shapes.push_back(ModelShape{
            std::move(bottomShape.vertices),
            std::move(bottomShape.indices),
            static_cast<uint16_t>(model.materials.size())
        });
        model.materials.push_back({
            .name = "plain_bottom",
            .parameters = {
                .albedo = sRGB(Float3::One() * 0.1f).toFloat3()
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

        const int faceCount =
            (hasEntry + hasExit) * CylinderEntryExitStrips * (entryExitSubdivision - 1) +
            (cylinderStrips - 1) * subdivision;
        GroundShapeData topShape{faceCount};
        GroundShapeData bottomShape{faceCount};

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

                    pushGroundTopFace(
                        topShape,
                        l0, r0, l1, r1,
                        options);
                    pushGroundBottomFace(
                        bottomShape,
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

                pushGroundTopFace(
                    topShape,
                    l0, r0, l1, r1,
                    options);
                pushGroundBottomFace(
                    bottomShape,
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

                    pushGroundTopFace(
                        topShape,
                        l0, r0, l1, r1,
                        options);
                    pushGroundBottomFace(
                        bottomShape,
                        l0, r0, l1, r1,
                        options);
                }
            }
        }

        model.shapes.push_back(ModelShape{
            std::move(topShape.vertices),
            std::move(topShape.indices),
            static_cast<uint16_t>(model.materials.size())
        });
        model.materials.push_back({
            .name = "plain_top",
            .parameters = {
                .albedo = sRGB(Float3::One() * 0.5f).toFloat3()
            }
        });

        model.shapes.push_back(ModelShape{
            std::move(bottomShape.vertices),
            std::move(bottomShape.indices),
            static_cast<uint16_t>(model.materials.size())
        });
        model.materials.push_back({
            .name = "plain_bottom",
            .parameters = {
                .albedo = sRGB(Float3::One() * 0.1f).toFloat3()
            }
        });
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

        BuildGimmickModel(model, segment, options);

        return model;
    }
}
