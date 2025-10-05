#include "pch.h"
#include "CourseBuilder.h"

#include "CourseConstants.h"
#include "TY/Quaternion.h"
#include "TY/Shape3D.h"

using namespace Race;

namespace
{
    ModelBuffer buildRoadModel(const CourseSegment& segment, Array<Triangle3D>* outCollider)
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
                outCollider->push_back(Triangle3D{s1.leftmost, s0.leftmost, s1.rightmost});
                outCollider->push_back(Triangle3D{s1.rightmost, s0.leftmost, s0.rightmost});
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

    ModelBuffer buildTunnelModel(const CourseSegment& segment, Array<Triangle3D>* outCollider)
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
            for (int s = 0; s < subdivision; ++s)
            {
                const Float3 l0 = s0.center + n0s[s] * r;
                const Float3 r0 = s0.center + n0s[(s + 1) % subdivision] * r;
                const Float3 l1 = s1.center + n1s[s] * r;
                const Float3 r1 = s1.center + n1s[(s + 1) % subdivision] * r;

                vertices[v_offset] = ModelVertex{l1, -n1s[s] * r, Float2{}};
                vertices[v_offset + 1] = ModelVertex{r1, -n1s[(s + 1) % subdivision] * r, Float2{1, 0}};
                vertices[v_offset + 2] = ModelVertex{l0, -n0s[s], Float2{0, 1}};
                vertices[v_offset + 3] = ModelVertex{r0, -n1s[(s + 1) % subdivision], Float2{1, 1}};

                indices[i_offset] = v_offset;
                indices[i_offset + 1] = v_offset + 2;
                indices[i_offset + 2] = v_offset + 1;
                indices[i_offset + 3] = v_offset + 1;
                indices[i_offset + 4] = v_offset + 2;
                indices[i_offset + 5] = v_offset + 3;

                v_offset += 4;
                i_offset += 6;

                if (outCollider)
                {
                    outCollider->push_back(Triangle3D{l1, l0, r1});
                    outCollider->push_back(Triangle3D{r1, l0, r0});
                }
            }

            // 裏面
            for (int s = 0; s < subdivision; ++s)
            {
                const Float3 l0 = s0.center + n0s[s] * r;
                const Float3 r0 = s0.center + n0s[(s + 1) % subdivision] * r;
                const Float3 l1 = s1.center + n1s[s] * r;
                const Float3 r1 = s1.center + n1s[(s + 1) % subdivision] * r;

                vertices[v_offset] = ModelVertex{l1, n1s[s] * r, Float2{}};
                vertices[v_offset + 1] = ModelVertex{r1, n1s[(s + 1) % subdivision], Float2{1, 0}};
                vertices[v_offset + 2] = ModelVertex{l0, n0s[s], Float2{0, 1}};
                vertices[v_offset + 3] = ModelVertex{r0, n0s[(s + 1) % subdivision], Float2{1, 1}};

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
    ModelBuffer BuildCourseModel(const CourseSegment& segment, Array<Triangle3D>* outCollider)
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
