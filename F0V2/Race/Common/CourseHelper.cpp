#include "pch.h"
#include "CourseHelper.h"

#include "Asset0.h"
#include "TY/Graphics3D.h"
#include "TY/Shape3D.h"
#include "TY/ShapeDrawer.h"
#include "TY/Utils.h"

namespace Race
{
    ModelBuffer BuildCourseModel(const CourseSegment& segment)
    {
        assert(segment.midwayPositions.size() > 0);
        Array<ModelVertex> vertices((segment.midwayPositions.size() - 1) * 8);
        Array<uint16_t> indices((segment.midwayPositions.size() - 1) * 12);
        int v_offset{};
        int i_offset{};
        for (int m = 0; m < segment.midwayPositions.size() - 1; ++m)
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

    void DebugDrawCourse(const Array<CourseSegment>& segments)
    {
        // コース中心を線分で描画
        Shape3D::LineSet lineSet{};
        for (int i = 0; i < segments.size(); ++i)
        {
            const auto& segment = segments[i];
            for (int j = 0; j < segment.midwayPositions.size() - 1; ++j)
            {
                constexpr Float3 d{0, 0.1, 0};
                lineSet.appendLine(segment.midwayPositions[j] + d, segment.midwayPositions[j + 1] + d);
            }
        }

        lineSet.setColor(ColorF32{1.0f, 0.5f, 0.1f})
               .pushAuto();

        // インデックスをテキスト描画
        const auto worldToScreen = Graphics3D::WorldToScreen();
        for (int i = 0; i < segments.size(); ++i)
        {
            const auto& segment = segments[i];
            Shape2D_Text::MPlus1_16_Bitmap(ToUtf32(std::to_string(i)))
                .setPosition(worldToScreen.transformPoint(segment.p1).xy())
                .pushAuto();
        }

        ShapeDrawer::Global().draw();
    }
}
