#include "pch.h"
#include "CourseModelBuilder.h"

#include "CourseConstants.h"
#include "CourseMinimapModelBuilder.h"
#include "CourseTextureKind.h"
#include "GimmickModelBuilder.h"
#include "TY/Quaternion.h"
#include "TY/Immediate3D.h"
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

    // 上面の四角形から、厚みの分だけずらした下面の四角形を作る
    FaceQuad makeBottomFaceQuad(const FaceQuad& top)
    {
        const auto toBottom = [](const FaceVertex& v)
        {
            return FaceVertex{v.pos - v.normal * bottomThickness, -v.normal, v.metadata};
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

    struct GroundShapeData
    {
        Array<CourseModelVertex> vertices;
        int vertexOffset{};

        Array<uint16_t> indices;
        int indexOffset{};

        explicit GroundShapeData(int faceCount)
            : vertices(faceCount * 4),
              indices(faceCount * 6)
        {
        }
    };

    CourseTextureKind faceTypeToTextureKind(const CourseFaceType faceType)
    {
        switch (faceType)
        {
        case CourseFaceType::RoadTop: return CourseTextureKind::RoadTop;
        case CourseFaceType::RoadBottom: return CourseTextureKind::RoadBottom;
        case CourseFaceType::RoadSide: return CourseTextureKind::RoadSide;
        default: return CourseTextureKind::None; // TODO: パイプ・シリンダー・バリア用のテクスチャ
        }
    }

    uint32_t getTextureIndex(const CourseFaceType faceType)
    {
        return static_cast<uint32_t>(faceTypeToTextureKind(faceType));
    }

    void pushGroundTopFace(
        GroundShapeData& shape,
        const FaceQuad& face,
        const CourseFaceType faceType,
        const CourseModelBuilderOptions& options,
        const RectF& uvRect = RectF{0, 0, 1, 1},
        const bool splitCenter = false, // TODO: 分割数を指定する?
        const CourseTextureKind overrideTexture = CourseTextureKind::None)
    {
        const auto& [l0, r0, l1, r1] = face;
        const auto f = static_cast<uint32_t>(faceType);
        const auto t = overrideTexture != CourseTextureKind::None
                           ? static_cast<int>(overrideTexture)
                           : getTextureIndex(faceType);

        // NOTE: 面を分割することで矩形の UV 補完精度が向上する
        std::array<FaceQuad, 2> subFaces{face};
        std::array<RectF, 2> subUVRects{uvRect};
        int subFaceCount = 1;
        if (splitCenter)
        {
            const FaceVertex c0{
                (l0.pos + r0.pos) * 0.5f, (l0.normal + r0.normal).normalized(), (l0.metadata + r0.metadata) * 0.5f
            };
            const FaceVertex c1{
                (l1.pos + r1.pos) * 0.5f, (l1.normal + r1.normal).normalized(), (l1.metadata + r1.metadata) * 0.5f
            };

            const float halfW = uvRect.w * 0.5f;
            subFaces = {FaceQuad{l0, c0, l1, c1}, FaceQuad{c0, r0, c1, r1}};
            subUVRects = {
                RectF{uvRect.x + halfW, uvRect.y, halfW, uvRect.h},
                RectF{uvRect.x, uvRect.y, halfW, uvRect.h}
            };
            subFaceCount = 2;
        }

        for (int i = 0; i < subFaceCount; ++i)
        {
            const auto& [sl0, sr0, sl1, sr1] = subFaces[i];
            const RectF& subUV = subUVRects[i];

            shape.vertices[shape.vertexOffset] = CourseModelVertex{
                sr1.pos, sr1.normal, subUV.bl(), f, t, sr1.metadata
            };
            shape.vertices[shape.vertexOffset + 1] = CourseModelVertex{
                sl1.pos, sl1.normal, subUV.br(), f, t, sl1.metadata
            };
            shape.vertices[shape.vertexOffset + 2] = CourseModelVertex{
                sr0.pos, sr0.normal, subUV.tl(), f, t, sr0.metadata
            };
            shape.vertices[shape.vertexOffset + 3] = CourseModelVertex{
                sl0.pos, sl0.normal, subUV.tr(), f, t, sl0.metadata
            };

            shape.indices[shape.indexOffset] = shape.vertexOffset;
            shape.indices[shape.indexOffset + 1] = shape.vertexOffset + 2;
            shape.indices[shape.indexOffset + 2] = shape.vertexOffset + 1;
            shape.indices[shape.indexOffset + 3] = shape.vertexOffset + 1;
            shape.indices[shape.indexOffset + 4] = shape.vertexOffset + 2;
            shape.indices[shape.indexOffset + 5] = shape.vertexOffset + 3;

            shape.vertexOffset += 4;
            shape.indexOffset += 6;
        }

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
        const FaceQuad& face,
        const CourseFaceType faceType,
        const CourseModelBuilderOptions& options,
        const RectF& uvRect = RectF{0, 0, 1, 1},
        const CourseTextureKind overrideTexture = CourseTextureKind::None)
    {
        const auto& [l0, r0, l1, r1] = face;
        const auto f = static_cast<uint32_t>(faceType);
        const auto t = overrideTexture != CourseTextureKind::None
                           ? static_cast<int>(overrideTexture)
                           : getTextureIndex(faceType);

        shape.vertices[shape.vertexOffset] = CourseModelVertex{r1.pos, r1.normal, uvRect.bl(), f, t, r1.metadata};
        shape.vertices[shape.vertexOffset + 1] = CourseModelVertex{l1.pos, l1.normal, uvRect.br(), f, t, l1.metadata};
        shape.vertices[shape.vertexOffset + 2] = CourseModelVertex{r0.pos, r0.normal, uvRect.tl(), f, t, r0.metadata};
        shape.vertices[shape.vertexOffset + 3] = CourseModelVertex{l0.pos, l0.normal, uvRect.tr(), f, t, l0.metadata};

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

    // 上面と下面の間の隙間を埋める側面
    void pushGroundSideFace(
        GroundShapeData& shape,
        const FaceQuad& face,
        const CourseFaceType faceType,
        const RectF& uvRect = RectF{0, 0, 1, 1},
        const CourseTextureKind overrideTexture = CourseTextureKind::None)
    {
        const auto& [l0, r0, l1, r1] = face;
        const auto f = static_cast<uint32_t>(faceType);
        const auto t = overrideTexture != CourseTextureKind::None
                           ? static_cast<int>(overrideTexture)
                           : getTextureIndex(faceType);

        shape.vertices[shape.vertexOffset] = CourseModelVertex{r1.pos, r1.normal, uvRect.bl(), f, t, r1.metadata};
        shape.vertices[shape.vertexOffset + 1] = CourseModelVertex{l1.pos, l1.normal, uvRect.br(), f, t, l1.metadata};
        shape.vertices[shape.vertexOffset + 2] = CourseModelVertex{r0.pos, r0.normal, uvRect.tl(), f, t, r0.metadata};
        shape.vertices[shape.vertexOffset + 3] = CourseModelVertex{l0.pos, l0.normal, uvRect.tr(), f, t, l0.metadata};

        shape.indices[shape.indexOffset] = shape.vertexOffset;
        shape.indices[shape.indexOffset + 1] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 2] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 3] = shape.vertexOffset + 1;
        shape.indices[shape.indexOffset + 4] = shape.vertexOffset + 2;
        shape.indices[shape.indexOffset + 5] = shape.vertexOffset + 3;

        shape.vertexOffset += 4;
        shape.indexOffset += 6;
    }

    void addGroundSideShape(CourseModelData& model, GroundShapeData& sideShape)
    {
        model.shapes.push_back(CourseModelShape{
            std::move(sideShape.vertices),
            std::move(sideShape.indices),
        });
    }

    void buildRoadModel(
        CourseModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        const bool createStartingLine = options.createStartingLine;
        constexpr int startingLineStripCount = 2;

        // ガードレールがある場合は側面の隙間が隠れるので、側面は不要
        const bool needsSideFace = not segment.gimmicks.contains(CourseGimmickKind::Barrier);

        // 前後が Gap の場合は断面が見えるので塞ぐ
        const bool needsFrontCap = options.priorStyle == CourseSegmentStyle::Gap;
        const bool needsBackCap = options.nextStyle == CourseSegmentStyle::Gap;
        const int lastStrip = static_cast<int>(segment.midwayStrips.size()) - 2;

        const int sideFaceCount =
            (needsSideFace ? (lastStrip + 1) * 2 : 0) + needsFrontCap + needsBackCap;
        GroundShapeData sideShape{sideFaceCount};

        {
            const int m0 = createStartingLine ? startingLineStripCount : 0;
            const int faceCount = static_cast<int>(segment.midwayStrips.size()) - 1 - m0;
            constexpr int subFaces = 2;
            GroundShapeData topShape{faceCount * subFaces};
            GroundShapeData bottomShape{faceCount}; // TODO: topShape, bottomShape で分ける意味が無くなったので統合する

            float vOffset = 0;
            for (int m = m0; m < segment.midwayStrips.size() - 1; ++m)
            {
                auto& s0 = segment.midwayStrips[m];
                auto& s1 = segment.midwayStrips[m + 1];

                // metadata に道幅を入れる
                const float w0 = (s0.rightmost - s0.leftmost).length();
                const float w1 = (s1.rightmost - s1.leftmost).length();

                const FaceQuad topFace{
                    {s0.leftmost, s0.normal, w0}, {s0.rightmost, s0.normal, w0},
                    {s1.leftmost, s1.normal, w1}, {s1.rightmost, s1.normal, w1}
                };
                const FaceQuad bottomFace = makeBottomFaceQuad(topFace);

                const float v1 = vOffset + s0.lengthToNext;
                const RectF roadUV = RectF{1.0f, vOffset, -2.0f, v1 - vOffset};

                pushGroundTopFace(topShape, topFace, CourseFaceType::RoadTop, options, roadUV, /* splitCenter */ true);
                pushGroundBottomFace(bottomShape, bottomFace, CourseFaceType::RoadBottom, options, roadUV);

                if (needsSideFace)
                {
                    pushGroundSideFace(sideShape, makeLeftSideFaceQuad(topFace, bottomFace), CourseFaceType::RoadSide);
                    pushGroundSideFace(sideShape, makeRightSideFaceQuad(topFace, bottomFace), CourseFaceType::RoadSide);
                }

                if (needsFrontCap && m == 0)
                {
                    pushGroundSideFace(sideShape, makeFrontCapFaceQuad(topFace, bottomFace), CourseFaceType::RoadSide);
                }

                if (needsBackCap && m == lastStrip)
                {
                    pushGroundSideFace(sideShape, makeBackCapFaceQuad(topFace, bottomFace), CourseFaceType::RoadSide);
                }

                vOffset = v1;
            }

            model.shapes.push_back(CourseModelShape{
                std::move(topShape.vertices),
                std::move(topShape.indices),
            });

            model.shapes.push_back(CourseModelShape{
                std::move(bottomShape.vertices),
                std::move(bottomShape.indices),
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

                const float w0 = (s0.rightmost - s0.leftmost).length();
                const float w1 = (s1.rightmost - s1.leftmost).length();

                const FaceQuad topFace{
                    {s0.leftmost, s0.normal, w0}, {s0.rightmost, s0.normal, w0},
                    {s1.leftmost, s1.normal, w1}, {s1.rightmost, s1.normal, w1}
                };
                const FaceQuad bottomFace = makeBottomFaceQuad(topFace);

                if (m == 0)
                {
                    assert((s1.center - s0.center).length()>0);
                    texW = texH * w0 / (s1.center - s0.center).length();
                }

                const RectF uvRect{0.0f, texH * m, texW, texH};
                pushGroundTopFace(
                    topShape, topFace, CourseFaceType::Default, options, uvRect, /* splitCenter */ false,
                    CourseTextureKind::StartingLine);
                pushGroundBottomFace(bottomShape, bottomFace, CourseFaceType::RoadBottom, options, uvRect);

                if (needsSideFace)
                {
                    pushGroundSideFace(sideShape, makeLeftSideFaceQuad(topFace, bottomFace), CourseFaceType::RoadSide);
                    pushGroundSideFace(sideShape, makeRightSideFaceQuad(topFace, bottomFace), CourseFaceType::RoadSide);
                }

                if (needsFrontCap && m == 0)
                {
                    pushGroundSideFace(sideShape, makeFrontCapFaceQuad(topFace, bottomFace), CourseFaceType::RoadSide);
                }

                if (needsBackCap && m == lastStrip)
                {
                    pushGroundSideFace(sideShape, makeBackCapFaceQuad(topFace, bottomFace), CourseFaceType::RoadSide);
                }
            }

            model.shapes.push_back(CourseModelShape{
                std::move(topShape.vertices),
                std::move(topShape.indices),
            });

            model.shapes.push_back(CourseModelShape{
                std::move(bottomShape.vertices),
                std::move(bottomShape.indices),
            });
        }

        if (sideFaceCount > 0)
        {
            addGroundSideShape(model, sideShape);
        }
    }

    void buildPipeModel(
        CourseModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options)
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
        // 出入り口は円周が閉じていないので、両端に側面が必要
        // さらに、円周のうち出入り口と繋がっていない上半分の断面を塞ぐ面が必要
        const int sideFaceCount = (hasEntry + hasExit) * (PipeEntryExitStrips * 2 + (subdivision - halfSubdivision0));
        GroundShapeData topShape{faceCount};
        GroundShapeData bottomShape{faceCount};
        GroundShapeData sideShape{sideFaceCount};

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

                    const FaceQuad topFace{l0, r0, l1, r1};
                    const FaceQuad bottomFace = makeBottomFaceQuad(topFace);

                    pushGroundTopFace(topShape, topFace, CourseFaceType::PipeEntryExitTop, options);
                    pushGroundBottomFace(bottomShape, bottomFace, CourseFaceType::PipeEntryExitBottom, options);

                    if (i0 == 0)
                    {
                        pushGroundSideFace(sideShape, makeLeftSideFaceQuad(topFace, bottomFace),
                                           CourseFaceType::PipeEntryExitSide);
                    }
                    if (i1 == halfSubdivision1 - 1)
                    {
                        pushGroundSideFace(sideShape, makeRightSideFaceQuad(topFace, bottomFace),
                                           CourseFaceType::PipeEntryExitSide);
                    }
                }
            }
        }

        const int pipeFirstStrip = hasEntry * PipeEntryExitStrips;
        const int pipeLastStrip = pipeFirstStrip + pipeStrips - 2;
        for (int m = pipeFirstStrip; m <= pipeLastStrip; ++m)
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

                const FaceQuad topFace{l0, r0, l1, r1};
                const FaceQuad bottomFace = makeBottomFaceQuad(topFace);

                pushGroundTopFace(topShape, topFace, CourseFaceType::PipeInner, options);
                pushGroundBottomFace(bottomShape, bottomFace, CourseFaceType::PipeOuter, options);

                // 出入り口と繋がっていない上半分は断面が開いているので塞ぐ
                if (i0 >= halfSubdivision0)
                {
                    if (hasEntry && m == pipeFirstStrip)
                    {
                        pushGroundSideFace(sideShape, makeFrontCapFaceQuad(topFace, bottomFace),
                                           CourseFaceType::PipeCap);
                    }
                    if (hasExit && m == pipeLastStrip)
                    {
                        pushGroundSideFace(sideShape, makeBackCapFaceQuad(topFace, bottomFace),
                                           CourseFaceType::PipeCap);
                    }
                }
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

                    const FaceQuad topFace{l0, r0, l1, r1};
                    const FaceQuad bottomFace = makeBottomFaceQuad(topFace);

                    pushGroundTopFace(topShape, topFace, CourseFaceType::PipeEntryExitTop, options);
                    pushGroundBottomFace(bottomShape, bottomFace, CourseFaceType::PipeEntryExitBottom, options);

                    if (i0 == 0)
                    {
                        pushGroundSideFace(sideShape, makeLeftSideFaceQuad(topFace, bottomFace),
                                           CourseFaceType::PipeEntryExitSide);
                    }
                    if (i1 == halfSubdivision1 - 1)
                    {
                        pushGroundSideFace(sideShape, makeRightSideFaceQuad(topFace, bottomFace),
                                           CourseFaceType::PipeEntryExitSide);
                    }
                }
            }
        }

        model.shapes.push_back(CourseModelShape{
            std::move(topShape.vertices),
            std::move(topShape.indices),
        });

        model.shapes.push_back(CourseModelShape{
            std::move(bottomShape.vertices),
            std::move(bottomShape.indices),
        });

        if (sideFaceCount > 0)
        {
            addGroundSideShape(model, sideShape);
        }
    }

    void buildCylinderModel(CourseModelData& model, const CourseSegment& segment,
                            const CourseModelBuilderOptions& options)
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
        // 出入り口は円周が閉じていないので、両端に側面が必要
        // さらに、出入り口のシリンダー側の端の断面を塞ぐ面が必要
        const int sideFaceCount =
            (hasEntry + hasExit) * (CylinderEntryExitStrips * 2 + (entryExitSubdivision - 1));
        GroundShapeData topShape{faceCount};
        GroundShapeData bottomShape{faceCount};
        GroundShapeData sideShape{sideFaceCount};

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

                    const FaceQuad topFace{l0, r0, l1, r1};
                    const FaceQuad bottomFace = makeBottomFaceQuad(topFace);

                    pushGroundTopFace(topShape, topFace, CourseFaceType::CylinderEntryExitTop, options);
                    pushGroundBottomFace(bottomShape, bottomFace, CourseFaceType::CylinderEntryExitBottom, options);

                    if (i0 == 0)
                    {
                        pushGroundSideFace(sideShape, makeLeftSideFaceQuad(topFace, bottomFace),
                                           CourseFaceType::CylinderEntryExitSide);
                    }
                    if (i1 == entryExitSubdivision - 1)
                    {
                        pushGroundSideFace(sideShape, makeRightSideFaceQuad(topFace, bottomFace),
                                           CourseFaceType::CylinderEntryExitSide);
                    }

                    // シリンダー側の端の断面を塞ぐ
                    if (s == CylinderEntryExitStrips - 1)
                    {
                        pushGroundSideFace(sideShape, makeBackCapFaceQuad(topFace, bottomFace),
                                           CourseFaceType::CylinderEntryExitCap);
                    }
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

                const FaceQuad topFace{l0, r0, l1, r1};

                pushGroundTopFace(topShape, topFace, CourseFaceType::CylinderOuter, options);
                // pushGroundBottomFace( // Bottom は見えない
                //     bottomShape, makeBottomFaceQuad(topFace), options);
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

                    const FaceQuad topFace{l0, r0, l1, r1};
                    const FaceQuad bottomFace = makeBottomFaceQuad(topFace);

                    pushGroundTopFace(topShape, topFace, CourseFaceType::CylinderEntryExitTop, options);
                    pushGroundBottomFace(bottomShape, bottomFace, CourseFaceType::CylinderEntryExitBottom, options);

                    if (i0 == 0)
                    {
                        pushGroundSideFace(sideShape, makeLeftSideFaceQuad(topFace, bottomFace),
                                           CourseFaceType::CylinderEntryExitSide);
                    }
                    if (i1 == entryExitSubdivision - 1)
                    {
                        pushGroundSideFace(sideShape, makeRightSideFaceQuad(topFace, bottomFace),
                                           CourseFaceType::CylinderEntryExitSide);
                    }

                    // シリンダー側の端の断面を塞ぐ
                    if (s == 0)
                    {
                        pushGroundSideFace(sideShape, makeFrontCapFaceQuad(topFace, bottomFace),
                                           CourseFaceType::CylinderEntryExitCap);
                    }
                }
            }
        }

        model.shapes.push_back(CourseModelShape{
            std::move(topShape.vertices),
            std::move(topShape.indices),
        });

        model.shapes.push_back(CourseModelShape{
            std::move(bottomShape.vertices),
            std::move(bottomShape.indices),
        });

        if (sideFaceCount > 0)
        {
            addGroundSideShape(model, sideShape);
        }
    }
}

namespace Race
{
    CourseModelData BuildCourseModel(const CourseSegment& segment, const CourseModelBuilderOptions& options)
    {
        assert(segment.midwayStrips.size() > 0);

        CourseModelData model{};

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
