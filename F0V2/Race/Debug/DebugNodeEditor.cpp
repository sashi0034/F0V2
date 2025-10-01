#include "pch.h"
#include "DebugNodeEditor.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "DebugEditorState.h"
#include "TY/ActorContainer.h"
#include "TY/Graphics3D.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/Shape2D.h"
#include "TY/Shape3D.h"
#include "TY/ShapeDrawer.h"
#include "TY/Utils.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
    // Catmull-Rom 補間 (区間 p1 --> p2)
    Float3 CatmullRom(const Float3& p0, const Float3& p1, const Float3& p2, const Float3& p3, float t)
    {
        float t2 = t * t;
        float t3 = t2 * t;

        return (p1 * 2.0f +
            (p2 - p0) * t +
            (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
            (-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3) * 0.5f;
    }

    // 点列を補完して返す
    Array<Float3> generateCatmullRomPoints(
        const Float3& p0, const Float3& p1, const Float3& p2, const Float3& p3, int samplesPerSegment)
    {
        Array<Float3> result;

        result.push_back(p1);

        for (int j = 1; j < samplesPerSegment; ++j)
        {
            float t = static_cast<float>(j) / samplesPerSegment;
            result.push_back(CatmullRom(p0, p1, p2, p3, t));
        }

        result.push_back(p2);

        return result;
    }

    struct CourseStrip
    {
        Float3 center;
        Float3 leftmost;
        Float3 rightmost;

        Float3 toNext; // 次点へのベクトル
        Float3 normal;
    };

    struct CourseSegment
    {
        Float3 p1{};
        Float3 p2{};

        Array<Float3> midwayPositions{};
        Array<CourseStrip> midwayStrips{};

        ModelDrawer drawer{};
    };
}

struct DebugNodeEditor::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    Array<CourseSegment> m_segments{};

    void Init()
    {
        const auto model = PrimitiveModel3D::Torus(1.0f, 0.5f, ColorF32{1.0f, 0.5f, 0.1f});

        m_drawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({g_debugEditorState->lambert});
    }

private:
    void update() override
    {
        nodeEditor();

        buildSegmentsIfNeeded();

        // コースの節点部分をトーラスで描画
        for (int i = 0; i < m_segments.size(); ++i)
        {
            const auto& pos = m_segments[i].p1;
            m_drawer.uploadWorldMatrix(Mat4x4::Translate(pos)).draw();
        }

        // 面描画
        for (int i = 0; i < m_segments.size(); ++i)
        {
            m_segments[i].drawer.draw();
        }

        // コース中心を線分で描画
        Shape3D::LineSet lineSet{};
        for (int i = 0; i < m_segments.size(); ++i)
        {
            const auto& segment = m_segments[i];
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
        for (int i = 0; i < m_segments.size(); ++i)
        {
            const auto& segment = m_segments[i];
            Shape2D_Text::MPlus1_16_Bitmap(ToUtf32(std::to_string(i)))
                .setPosition(worldToScreen.transformPoint(segment.p1).xy())
                .pushAuto();
        }

        ShapeDrawer::Global().draw();
    }

    void buildSegmentsIfNeeded()
    {
        auto& nodeList = g_debugEditorState->course.nodes;
        Array<int> rebuildIndex{};
        for (int i = 0; i < nodeList.size(); ++i)
        {
            auto& p0 = nodeList[(i - 1 + nodeList.size()) % nodeList.size()].pos;
            auto& p1 = nodeList[i].pos;
            auto& p2 = nodeList[(i + 1) % nodeList.size()].pos;
            auto& p3 = nodeList[(i + 2) % nodeList.size()].pos;
            if (i >= m_segments.size() || (m_segments[i].p1 != p1 || m_segments[i].p2 != p2))
            {
                if (i >= m_segments.size())
                {
                    m_segments.push_back({});
                }

                auto& segment = m_segments[i];
                segment.p1 = p1;
                segment.p2 = p2;
                segment.midwayPositions = generateCatmullRomPoints(p0, p1, p2, p3, 10);
                rebuildIndex.push_back(i);
            }
        }

        while (m_segments.size() > nodeList.size())
        {
            m_segments.pop_back();
        }

        // -----------------------------------------------

        // 変更があった CourseSegment の線分に対して面を構築する
        for (const auto i : rebuildIndex)
        {
            auto& segment = m_segments[i];
            segment.midwayStrips.clear();
            for (int m = 0; m < segment.midwayPositions.size() - 1 /* 終端は除外 */; ++m)
            {
                CourseStrip strip{};
                strip.center = segment.midwayPositions[m];
                strip.normal = Float3(0, 1, 0); // TODO

                auto nextPosition = segment.midwayPositions[m + 1];
                strip.toNext = nextPosition - strip.center;

                auto right = strip.toNext.cross(strip.normal).normalized();
                float width = 7.5f; // TODO
                strip.leftmost = strip.center - right * width;
                strip.rightmost = strip.center + right * width;

                segment.midwayStrips.push_back(strip);
            }

            // 終端部分は次のセクションで行う
        }

        for (const auto i : rebuildIndex)
        {
            auto& segment = m_segments[i];

            // 終端部分の追加
            {
                // TODO: バグ修正
                auto& segment1 = m_segments[(i + 1) % m_segments.size()];
                segment.midwayStrips.push_back(segment1.midwayStrips[0]);
            }

            assert(m_segments.size() > 0);
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
            segment.drawer =
                ModelDrawerParams{}
                .setModel(modelBuffer)
                .setShader(Asset_shader::lambert)
                .setCbv10AndLater({g_debugEditorState->lambert});
        }
    }

    void nodeEditor()
    {
        ImGui::Begin("Node Editor");

        Array<int> removeIndex{};
        auto& nodeList = g_debugEditorState->course.nodes;
        for (size_t i = 0; i < nodeList.size(); i++)
        {
            ImGui::Text(std::format("--- [{}] ---", i).c_str());

            ImGui::SameLine();

            if (ImGui::Button(std::format("Delete##{}", i).c_str()))
            {
                removeIndex.push_back(static_cast<int>(i));
            }

            ImGui::DragFloat3(std::format("Position##{}", i).c_str(), &nodeList[i].pos.x, 0.1f);
        }

        for (auto it = removeIndex.rbegin(); it != removeIndex.rend(); ++it)
        {
            if (InRange(*it, 0, static_cast<int>(nodeList.size() - 1)))
            {
                nodeList.erase(nodeList.begin() + *it);
            }
        }

        ImGui::SeparatorText("Operations");

        if (ImGui::Button("Add Node"))
        {
            auto lastElement = nodeList.empty() ? CourseNode{} : nodeList.back();
            lastElement.pos.z += 10.0f;
            nodeList.push_back(lastElement);
        }

        ImGui::End();
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"DebugNodeEditor";
    }
};

namespace Race
{
    DebugNodeEditor::DebugNodeEditor() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void DebugNodeEditor::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> DebugNodeEditor::asGameObject() const
    {
        return p_impl;
    }
}
