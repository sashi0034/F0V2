#include "pch.h"
#include "EditorNodeTool.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "ColorPalette.h"
#include "DebugUI.h"
#include "EditorState.h"
#include "Race/Common/CourseHelper.h"
#include "Race/Common/CourseData.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/Graphics3D.h"
#include "TY/Intersects2D.h"
#include "TY/ModelDrawer.h"
#include "TY/Mouse.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/Scene.h"
#include "TY/Shape2D.h"
#include "TY/Shape3D.h"
#include "TY/ShapeDrawer.h"
#include "TY/Utils.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Editor;
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
}

struct EditorNodeTool::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_torusDrawer{};

    Array<CourseSegment> m_segments{};

    Array<ModelDrawer> m_courseDrawers{};

    int m_activeNodeIndex{};

    void Init()
    {
        const auto model = PrimitiveModel3D::Torus(1.0f, 0.5f, ColorF32{1.0f, 0.5f, 0.1f});

        m_torusDrawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({g_editorState->lambert});
    }

private:
    void update() override
    {
        nodeTool();

        buildSegmentsIfNeeded();

        // コースの節点部分をトーラスで描画
        for (int i = 0; i < m_segments.size(); ++i)
        {
            const auto& pos = m_segments[i].p1;
            m_torusDrawer.uploadWorldMatrix(Mat4x4::Translate(pos)).draw();
        }

        // 面描画
        for (int i = 0; i < m_courseDrawers.size(); ++i)
        {
            m_courseDrawers[i].draw();
        }

        courseDebugUI(m_segments);
    }

    void courseDebugUI(const Array<CourseSegment>& segments)
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

        // -----------------------------------------------

        // インデックスをテキスト描画
        const auto worldToScreen = Graphics3D::WorldToScreen();
        for (int i = 0; i < segments.size(); ++i)
        {
            const auto& segment = segments[i];
            auto p1InScreen = worldToScreen.transformPoint(segment.p1);
            if (not InRange(p1InScreen.z, 0.0f, 1.0f))
            {
                continue;
            }

            const auto buttonRect =
                RectF{p1InScreen.xy(), Float2{}}.stretched(16);

            if (MouseL.down() && Intersects(Mouse::PosF(), buttonRect))
            {
                m_activeNodeIndex = i;
            }

            if (i == m_activeNodeIndex)
            {
                Shape2D::RoundRect{buttonRect}.setColor(ColorPalette::GamingGreen).pushAuto();
            }

            Shape2D_Text::MPlus1_16_Bitmap(ToUtf32(std::to_string(i)))
                .setPosition(p1InScreen.xy(), Alignment9::MiddleCenter)
                .setColor(i == m_activeNodeIndex ? ColorF32{0.0f} : ColorF32{1.0f})
                .pushAuto();
        }

        // -----------------------------------------------

        if (InRange(m_activeNodeIndex, 0, static_cast<int>(segments.size() - 1)))
        {
            auto text =
                std::format("[{}] {:.2f}, {:.2f}, {:.2f}",
                            m_activeNodeIndex,
                            segments[m_activeNodeIndex].p1.x,
                            segments[m_activeNodeIndex].p1.y,
                            segments[m_activeNodeIndex].p1.z);

            if (DebugUI::Button(RectF{Scene::Center(), Float2{240, 24}}, ToUtf32(text)))
            {
            }
        }

        // -----------------------------------------------

        ShapeDrawer::Global().draw();
    }

    void buildSegmentsIfNeeded()
    {
        auto& nodeList = g_editorState->course.nodes;
        Array<int> rebuildIndex{};
        for (int i = 0; i < nodeList.size(); ++i)
        {
            auto& p0 = nodeList[(i - 1 + nodeList.size()) % nodeList.size()].pos;
            auto& p1 = nodeList[i].pos;
            auto& p2 = nodeList[(i + 1) % nodeList.size()].pos;
            auto& p3 = nodeList[(i + 2) % nodeList.size()].pos;
            if (i >= m_segments.size() ||
                m_segments[i].p1 != p1 ||
                m_segments[i].p2 != p2)
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

                auto nextPosition = segment.midwayPositions[m + 1];
                strip.toNext = nextPosition - strip.center;

                {
                    const Float3 n = strip.toNext.cross(Float3(0, 1, 0));
                    strip.normal = n.cross(strip.toNext).normalized(); // 鉛直上ベクトルと進行方向に垂直なベクトル
                }

                auto right = strip.toNext.cross(strip.normal).normalized();
                float width = 7.5f; // TODO
                strip.leftmost = strip.center - right * width;
                strip.rightmost = strip.center + right * width;

                segment.midwayStrips.push_back(strip);
            }

            // 終端部分は次のセクションで行う
        }

        if (m_courseDrawers.size() != m_segments.size())
        {
            m_courseDrawers.resize(m_segments.size());
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

            const auto modelBuffer = BuildCourseModel(segment);
            m_courseDrawers[i] =
                ModelDrawerParams{}
                .setModel(modelBuffer)
                .setShader(Asset_shader::lambert)
                .setCbv10AndLater({g_editorState->lambert});
        }
    }

    void nodeTool()
    {
        ImGui::Begin("Node Tool");

        Array<int> removeIndex{};
        auto& nodeList = g_editorState->course.nodes;
        for (int i = 0; i < nodeList.size(); i++)
        {
            if (m_activeNodeIndex == i)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ColorPalette::GamingGreen.toFloat4().cast<ImVec4>());
            }

            ImGui::Text(std::format("--- [{}] ---", i).c_str());

            if (m_activeNodeIndex == i)
            {
                ImGui::PopStyleColor();
            }

            ImGui::SameLine();

            if (ImGui::Button(std::format("*##{}", i).c_str()))
            {
                m_activeNodeIndex = i;
            }

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

        if (ImGui::CollapsingHeader("Critical Operations"))
        {
            if (ImGui::Button("Rebuild Segments"))
            {
                m_segments.clear();
                buildSegmentsIfNeeded();
            }
        }

        ImGui::End();
    }

    float orderPriority() const override
    {
        return -100.0f;
    }

    void killed() override
    {
        m_children.killEach();

        g_sharedState->courseSegments = std::move(m_segments);
    }

    std::u32string name() const override
    {
        return U"EditorNodeTool";
    }
};

namespace Editor
{
    EditorNodeTool::EditorNodeTool() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void EditorNodeTool::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> EditorNodeTool::asGameObject() const
    {
        return p_impl;
    }
}
