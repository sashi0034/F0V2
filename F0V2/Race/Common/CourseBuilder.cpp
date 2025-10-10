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

    ModelBuffer buildTunnelModel(const CourseSegment& segment, CoursePolygoneCollider* outCollider)
    {
        constexpr int subdivision = TunnelSubdivision;
        Array<ModelVertex> vertices((segment.midwayStrips.size() - 1) * (subdivision * 4 * 2));
        Array<uint16_t> indices((segment.midwayStrips.size() - 1) * (subdivision * 6 * 2));
        int v_offset{};
        int i_offset{};
        for (int m = 0; m < segment.midwayStrips.size() - 2 /* TODO: 終端部分の調整 */ ; ++m)
        {
            auto& s0 = segment.midwayStrips[m];
            auto& s1 = segment.midwayStrips[m + 1];

            constexpr float r = 25.0f; // TODO

            std::array<Float3, subdivision> n0s = s0.tunnel.ringVectors;
            std::array<Float3, subdivision> n1s = s1.tunnel.ringVectors;

            // 表面
            for (int i0 = 0; i0 < subdivision; ++i0)
            {
                const int i1 = (i0 + 1) % subdivision;

                const Float3 l0 = s0.center + n0s[i0] * r;
                const Float3 r0 = s0.center + n0s[i1] * r;
                const Float3 l1 = s1.center + n1s[i0] * r;
                const Float3 r1 = s1.center + n1s[i1] * r;

                const Float3 l0_n = -n0s[i0];
                const Float3 r0_n = -n0s[i1];
                const Float3 l1_n = -n1s[i0];
                const Float3 r1_n = -n1s[i1];

                vertices[v_offset] = ModelVertex{l1, l1_n, Float2{}};
                vertices[v_offset + 1] = ModelVertex{r1, r1_n, Float2{1, 0}};
                vertices[v_offset + 2] = ModelVertex{l0, l0_n, Float2{0, 1}};
                vertices[v_offset + 3] = ModelVertex{r0, r0_n, Float2{1, 1}};

                indices[i_offset] = v_offset; // l1
                indices[i_offset + 1] = v_offset + 2; // l0
                indices[i_offset + 2] = v_offset + 1; // r1
                indices[i_offset + 3] = v_offset + 1;
                indices[i_offset + 4] = v_offset + 2;
                indices[i_offset + 5] = v_offset + 3;

                v_offset += 4;
                i_offset += 6;

                if (outCollider)
                {
                    const std::array normals_00_10_01_11{l1_n, r1_n, l0_n, r0_n};

                    outCollider->tris.push_back(IndexedTriangle{l0, l1, r0, outCollider->attributes.size()});
                    outCollider->attributes.push_back(CourseTriangleAttribute{
                        CourseTriangleAttribute::Triangle_01_00_11,
                        normals_00_10_01_11,
                        r1
                    });

                    outCollider->tris.push_back(IndexedTriangle{r0, l1, r1, outCollider->attributes.size()});
                    outCollider->attributes.push_back(CourseTriangleAttribute{
                        CourseTriangleAttribute::Triangle_11_00_10,
                        normals_00_10_01_11,
                        l0
                    });
                }
            }

            // 裏面
            for (int i0 = 0; i0 < subdivision; ++i0)
            {
                const int i1 = (i0 + 1) % subdivision;

                const Float3 l0 = s0.center + n0s[i0] * r;
                const Float3 r0 = s0.center + n0s[i1] * r;
                const Float3 l1 = s1.center + n1s[i0] * r;
                const Float3 r1 = s1.center + n1s[i1] * r;

                const Float3 l0_n = n0s[i0];
                const Float3 r0_n = n0s[i1];
                const Float3 l1_n = n1s[i0];
                const Float3 r1_n = n1s[i1];

                vertices[v_offset] = ModelVertex{l1, l1_n, Float2{}};
                vertices[v_offset + 1] = ModelVertex{r1, r1_n, Float2{1, 0}};
                vertices[v_offset + 2] = ModelVertex{l0, l0_n, Float2{0, 1}};
                vertices[v_offset + 3] = ModelVertex{r0, r0_n, Float2{1, 1}};

                indices[i_offset] = v_offset;
                indices[i_offset + 1] = v_offset + 1;
                indices[i_offset + 2] = v_offset + 2;
                indices[i_offset + 3] = v_offset + 1;
                indices[i_offset + 4] = v_offset + 3;
                indices[i_offset + 5] = v_offset + 2;

                v_offset += 4;
                i_offset += 6;
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
        else if (segment.style == CourseSegmentStyle::Tunnel)
        {
            return buildTunnelModel(segment, outCollider);
        }
        else
        {
            assert(false);
            return {};
        }
    }
}
