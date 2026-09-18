#include "pch.h"
#include "GimmickModelBuilder.h"

#include "CourseConstants.h"
#include "RaceSharedState.h"
#include "TY/Color.h"
#include "TY/Math.h"
#include "TY/Rect.h"

using namespace Race;

namespace
{
    constexpr float bottomThickness = 5.0f;

    struct FaceVertex
    {
        Float3 pos{};
        Float3 normal{};
        float metadata{};

        FaceVertex withNormal(const Float3 n) const
        {
            return FaceVertex{pos, n, metadata};
        }
    };

    // 0 -> 1 が進行方向、l -> r が左から右
    struct FaceQuad
    {
        FaceVertex l0{};
        FaceVertex r0{};
        FaceVertex l1{};
        FaceVertex r1{};
    };

    // 上面の四角形から、thickness の分だけずらした下面の四角形を作る
    FaceQuad makeBottomFaceQuad(const FaceQuad& top, float thickness)
    {
        const auto toBottom = [thickness](const FaceVertex& v)
        {
            return FaceVertex{v.pos - v.normal * thickness, -v.normal, v.metadata};
        };

        return FaceQuad{toBottom(top.l0), toBottom(top.r0), toBottom(top.l1), toBottom(top.r1)};
    }

    // 辺から外側へ向かう法線 (上面の法線と直交する成分)
    Float3 getOutwardNormal(const FaceVertex& edge, const FaceVertex& opposite)
    {
        const Float3 d = edge.pos - opposite.pos;
        return (d - edge.normal * d.dot(edge.normal)).normalized();
    }

    // 上面と下面の左端をつなぐ側面の四角形
    // 上面と巻き順を合わせるため、下の辺を l 側、上の辺を r 側とする
    FaceQuad makeLeftSideFaceQuad(const FaceQuad& top, const FaceQuad& bottom)
    {
        const Float3 n0 = getOutwardNormal(top.l0, top.r0);
        const Float3 n1 = getOutwardNormal(top.l1, top.r1);

        return FaceQuad{
            bottom.l0.withNormal(n0), top.l0.withNormal(n0),
            bottom.l1.withNormal(n1), top.l1.withNormal(n1)
        };
    }

    // 上面と下面の右端をつなぐ側面の四角形
    FaceQuad makeRightSideFaceQuad(const FaceQuad& top, const FaceQuad& bottom)
    {
        const Float3 n0 = getOutwardNormal(top.r0, top.l0);
        const Float3 n1 = getOutwardNormal(top.r1, top.l1);

        return FaceQuad{
            top.r0.withNormal(n0), bottom.r0.withNormal(n0),
            top.r1.withNormal(n1), bottom.r1.withNormal(n1)
        };
    }

    // 上面と下面の 0 側 (進行方向の手前) の断面を塞ぐ四角形
    // 上面と巻き順を合わせるため、下の辺を 0 側、上の辺を 1 側とする
    FaceQuad makeFrontCapFaceQuad(const FaceQuad& top, const FaceQuad& bottom)
    {
        const Float3 n = -((top.l1.pos + top.r1.pos) - (top.l0.pos + top.r0.pos)).normalized();

        return FaceQuad{
            bottom.l0.withNormal(n), bottom.r0.withNormal(n),
            top.l0.withNormal(n), top.r0.withNormal(n)
        };
    }

    // 上面と下面の 1 側 (進行方向の奥) の断面を塞ぐ四角形
    FaceQuad makeBackCapFaceQuad(const FaceQuad& top, const FaceQuad& bottom)
    {
        const Float3 n = ((top.l1.pos + top.r1.pos) - (top.l0.pos + top.r0.pos)).normalized();

        return FaceQuad{
            top.l1.withNormal(n), top.r1.withNormal(n),
            bottom.l1.withNormal(n), bottom.r1.withNormal(n)
        };
    }

    struct GimmickShapeData
    {
        Array<CourseModelVertex> vertices;
        int vertexOffset{};

        Array<uint16_t> indices;
        int indexOffset{};

        explicit GimmickShapeData(int faceCount)
            : vertices(faceCount * 4),
              indices(faceCount * 6)
        {
        }
    };

    void pushGimmickTopFace(
        GimmickShapeData& shape,
        int stripIndex,
        const FaceQuad& face,
        const CourseFaceType faceType,
        GimmickTriangleAttribute::kind_t gimmick,
        const CourseModelBuilderOptions& options,
        const RectF& uvRect = RectF{0, 0, 1, 1})
    {
        const auto& [l0, r0, l1, r1] = face;
        const auto t = static_cast<uint32_t>(faceType);

        shape.vertices[shape.vertexOffset] = CourseModelVertex{r1.pos, r1.normal, uvRect.bl(), t, r1.metadata};
        shape.vertices[shape.vertexOffset + 1] = CourseModelVertex{l1.pos, l1.normal, uvRect.br(), t, l1.metadata};
        shape.vertices[shape.vertexOffset + 2] = CourseModelVertex{r0.pos, r0.normal, uvRect.tl(), t, r0.metadata};
        shape.vertices[shape.vertexOffset + 3] = CourseModelVertex{l0.pos, l0.normal, uvRect.tr(), t, l0.metadata};

        shape.indices[shape.indexOffset] = shape.vertexOffset;
        shape.indices[shape.indexOffset + 1] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 2] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 3] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 4] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 5] = shape.vertexOffset + 3;

        shape.vertexOffset += 4;
        shape.indexOffset += 6;

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

    void pushGimmickBottomFace(
        GimmickShapeData& shape,
        const FaceQuad& face,
        const CourseFaceType faceType,
        const RectF& uvRect = RectF{0, 0, 1, 1})
    {
        const auto& [l0, r0, l1, r1] = face;
        const auto t = static_cast<uint32_t>(faceType);

        shape.vertices[shape.vertexOffset] = CourseModelVertex{r1.pos, r1.normal, uvRect.bl(), t, r1.metadata};
        shape.vertices[shape.vertexOffset + 1] = CourseModelVertex{l1.pos, l1.normal, uvRect.br(), t, l1.metadata};
        shape.vertices[shape.vertexOffset + 2] = CourseModelVertex{r0.pos, r0.normal, uvRect.tl(), t, r0.metadata};
        shape.vertices[shape.vertexOffset + 3] = CourseModelVertex{l0.pos, l0.normal, uvRect.tr(), t, l0.metadata};

        shape.indices[shape.indexOffset] = shape.vertexOffset;
        shape.indices[shape.indexOffset + 1] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 2] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 3] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 4] = shape.vertexOffset + 3;
        shape.indices[shape.indexOffset + 5] = shape.vertexOffset + 2;

        shape.vertexOffset += 4;
        shape.indexOffset += 6;
    }

    // 上面と下面の間の隙間を埋める側面
    void pushGimmickSideFace(
        GimmickShapeData& shape,
        const FaceQuad& face,
        const CourseFaceType faceType,
        const RectF& uvRect = RectF{0, 0, 1, 1})
    {
        const auto& [l0, r0, l1, r1] = face;
        const auto t = static_cast<uint32_t>(faceType);

        shape.vertices[shape.vertexOffset] = CourseModelVertex{r1.pos, r1.normal, uvRect.bl(), t, r1.metadata};
        shape.vertices[shape.vertexOffset + 1] = CourseModelVertex{l1.pos, l1.normal, uvRect.br(), t, l1.metadata};
        shape.vertices[shape.vertexOffset + 2] = CourseModelVertex{r0.pos, r0.normal, uvRect.tl(), t, r0.metadata};
        shape.vertices[shape.vertexOffset + 3] = CourseModelVertex{l0.pos, l0.normal, uvRect.tr(), t, l0.metadata};

        shape.indices[shape.indexOffset] = shape.vertexOffset;
        shape.indices[shape.indexOffset + 1] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 2] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 3] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 4] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 5] = shape.vertexOffset + 3;

        shape.vertexOffset += 4;
        shape.indexOffset += 6;
    }

    void buildBarrier_Road(
        CourseModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        constexpr float barrierHeight = 2.5f;
        constexpr float barrierThickness = 1.0f;

        const int lastStrip = static_cast<int>(segment.midwayStrips.size()) - 2;

        // 左右それぞれ、ストリップごとに上面・下面・側面 2 つ、最初と最後に断面
        GimmickShapeData shape{((lastStrip + 1) * 4 + 2) * 2};

        for (int m = 0; m <= lastStrip; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            const Float3 s0_l2r = (s0.rightmost - s0.leftmost).normalized();
            const Float3 s1_l2r = (s1.rightmost - s1.leftmost).normalized();

            // 道路側を向く面を上面とし、下端は地面の底面まで下げる
            // 上面と巻き順を合わせるため、左側は上端を l 側、右側は下端を l 側とする
            const FaceQuad leftTopFace{
                {s0.leftmost + s0.normal * barrierHeight, s0_l2r},
                {s0.leftmost - s0.normal * bottomThickness, s0_l2r},
                {s1.leftmost + s1.normal * barrierHeight, s1_l2r},
                {s1.leftmost - s1.normal * bottomThickness, s1_l2r},
            };
            const FaceQuad rightTopFace{
                {s0.rightmost - s0.normal * bottomThickness, -s0_l2r},
                {s0.rightmost + s0.normal * barrierHeight, -s0_l2r},
                {s1.rightmost - s1.normal * bottomThickness, -s1_l2r},
                {s1.rightmost + s1.normal * barrierHeight, -s1_l2r},
            };

            for (const FaceQuad& topFace : {leftTopFace, rightTopFace})
            {
                const FaceQuad bottomFace = makeBottomFaceQuad(topFace, barrierThickness);

                pushGimmickTopFace(
                    shape, m, topFace, CourseFaceType::BarrierTop, GimmickTriangleAttribute::kind_t::Barrier, options);
                pushGimmickBottomFace(shape, bottomFace, CourseFaceType::BarrierBottom);
                pushGimmickSideFace(shape, makeLeftSideFaceQuad(topFace, bottomFace), CourseFaceType::BarrierSide);
                pushGimmickSideFace(shape, makeRightSideFaceQuad(topFace, bottomFace), CourseFaceType::BarrierSide);

                // FIXME: 前後のセグメントに Barrier があるときは断面を塞がないようにする
                if (m == 0)
                {
                    pushGimmickSideFace(shape, makeFrontCapFaceQuad(topFace, bottomFace), CourseFaceType::BarrierSide);
                }
                if (m == lastStrip)
                {
                    pushGimmickSideFace(shape, makeBackCapFaceQuad(topFace, bottomFace), CourseFaceType::BarrierSide);
                }
            }
        }

        model.shapes.push_back(CourseModelShape{
            std::move(shape.vertices), std::move(shape.indices), model.takeMaterialIndex("plain")
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
        CourseModelData& model,
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

        const FaceQuad topFace{l0, r0, l1, r1};

        GimmickShapeData shape{2};
        pushGimmickTopFace(shape, s0_index, topFace, CourseFaceType::Default, gimmick, options);
        pushGimmickBottomFace(shape, makeBottomFaceQuad(topFace, 0.0f), CourseFaceType::Default);

        assert(gimmick == GimmickTriangleAttribute::kind_t::BoostPad ||
            gimmick == GimmickTriangleAttribute::kind_t::JumpPad);

        const uint16_t materialIndex =
            gimmick == GimmickTriangleAttribute::kind_t::BoostPad
                ? model.takeMaterialIndex(
                    "boost_pad", g_sharedState->courseTexture(CourseTextureKind::BoostPad).getFrontRtv())
                : model.takeMaterialIndex(
                    "jump_pad", g_sharedState->courseTexture(CourseTextureKind::JumpPad).getFrontRtv());

        model.shapes.push_back(CourseModelShape{
            std::move(shape.vertices), std::move(shape.indices), materialIndex
        });
    }

    void buildPad_Circular(
        CourseModelData& model,
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

        const FaceQuad topFace{l0, r0, l1, r1};

        GimmickShapeData shape{2};
        pushGimmickTopFace(shape, s0_index, topFace, CourseFaceType::Default, gimmick, options);
        pushGimmickBottomFace(shape, makeBottomFaceQuad(topFace, 0.0f), CourseFaceType::Default);

        assert(gimmick == GimmickTriangleAttribute::kind_t::BoostPad ||
            gimmick == GimmickTriangleAttribute::kind_t::JumpPad);

        const uint16_t materialIndex =
            gimmick == GimmickTriangleAttribute::kind_t::BoostPad
                ? model.takeMaterialIndex(
                    "boost_pad", g_sharedState->courseTexture(CourseTextureKind::BoostPad).getFrontRtv())
                : model.takeMaterialIndex(
                    "jump_pad", g_sharedState->courseTexture(CourseTextureKind::JumpPad).getFrontRtv());

        model.shapes.push_back(CourseModelShape{
            std::move(shape.vertices), std::move(shape.indices), materialIndex
        });
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

    void buildPitZone_Road(CourseModelData& model, const CourseSegment& segment, LCR lcr,
                           const CourseModelBuilderOptions& options)
    {
        constexpr float padElevation = 0.5f;

        const int s0_index = segment.midwayStrips.size() / 2 - 1;
        if (not InRange<int>(s0_index, 0, segment.midwayStrips.size() - 2))
        {
            return;
        }

        GimmickShapeData shape{(static_cast<int>(segment.midwayStrips.size()) - 1) * 2};

        float texY{};
        for (int m = 0; m < segment.midwayStrips.size() - 1; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            const auto lr0 = separateStrip(s0, lcr);
            const auto lr1 = separateStrip(s1, lcr);

            const FaceQuad topFace{
                {lr0.first + s0.normal * padElevation, s0.normal},
                {lr0.second + s0.normal * padElevation, s0.normal},
                {lr1.first + s1.normal * padElevation, s1.normal},
                {lr1.second + s1.normal * padElevation, s1.normal},
            };

            const float texH = 2.0f * (s1.center - s0.center).length() / (s0.rightmost - s0.leftmost).length();
            const RectF uvRect{0.0f, texY, 1.0f, texH};

            pushGimmickTopFace(
                shape, m, topFace, CourseFaceType::Default, GimmickTriangleAttribute::kind_t::PitZone, options, uvRect);
            pushGimmickBottomFace(shape, makeBottomFaceQuad(topFace, 0.0f), CourseFaceType::Default, uvRect);

            texY += texH;
        }

        model.shapes.push_back(CourseModelShape{
            std::move(shape.vertices), std::move(shape.indices),
            model.takeMaterialIndex(
                "pit_zone", g_sharedState->courseTexture(CourseTextureKind::PitZone).getFrontRtv())
        });
    }
}

void Race::BuildGimmickModel(
    CourseModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
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
            assert(false && "BuildGimmickModel(): gimmick kind is not supported.");
            break;
        }
    }
}
