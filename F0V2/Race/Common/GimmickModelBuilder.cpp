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
    struct FaceVertex
    {
        Float3 pos{};
        Float3 normal{};
    };

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
                .albedo = sRGB(0.97f, 0.53f, 0.00f).toFloat3()
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
                    .albedo = Float3::One()
                },
                .albedoTexture = g_sharedState->gimmickTextures.boostPad.getFrontRtv()
            });
        }
        else
        {
            assert(gimmick == GimmickTriangleAttribute::kind_t::JumpPad);
            model.materials.push_back({
                .name = "jump_pad",
                .parameters = {
                    .albedo = Float3::One()
                },
                .albedoTexture = g_sharedState->gimmickTextures.jumpPad.getFrontRtv()
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
                    .albedo = Float3::One()
                },
                .albedoTexture = g_sharedState->gimmickTextures.boostPad.getFrontRtv()
            });
        }
        else
        {
            assert(gimmick == GimmickTriangleAttribute::kind_t::JumpPad);
            model.materials.push_back({
                .name = "jump_pad",
                .parameters = {
                    .albedo = Float3::One()
                },
                .albedoTexture = g_sharedState->gimmickTextures.jumpPad.getFrontRtv()
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
                .albedo = Float3::One()
            },
            .albedoTexture = g_sharedState->gimmickTextures.pitZone.getFrontRtv()
        });
    }
}

void Race::BuildGimmickModel(ModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
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
