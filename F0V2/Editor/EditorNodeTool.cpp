#include "pch.h"
#include "EditorNodeTool.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "ColorPalette.h"
#include "Util/DebugUI.h"
#include "EditorState.h"
#include "Race/Common/CourseModelBuilder.h"
#include "Race/Common/CourseData.h"
#include "Race/Common/CourseSegmentBuilder.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/Graphics3D.h"
#include "TY/Intersects2D.h"
#include "TY/ModelDrawer.h"
#include "TY/Mouse.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/Scene.h"
#include "TY/Immediate2D.h"
#include "TY/Immediate3D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Utils.h"
#include "TY_Extension/GameObjectBase.h"
#include "Util/CatmullRom.h"
#include "Util/Utilities.h"

using namespace Editor;
using namespace Race;
using namespace Util;

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
        Immediate3D::LineSet lineSet{};
        for (int i = 0; i < segments.size(); ++i)
        {
            const auto& segment = segments[i];
            for (int j = 0; j < segment.midwayStrips.size() - 1; ++j)
            {
                const Float3 d0 = segment.midwayStrips[j].normal;
                const Float3 d1 = segment.midwayStrips[j + 1].normal;
                lineSet.appendLine(segment.midwayStrips[j].center + d0, segment.midwayStrips[j + 1].center + d1);
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
                Immediate2D::RoundRect{buttonRect}.setColor(ColorPalette::GamingGreen).pushAuto();
            }

            Immediate2D_Text::MPlus1_16_Bitmap(ToUtf32(std::to_string(i)))
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

            if (DebugUI::DragButton(RectF{Scene::Center(), Float2{240, 24}}, ToUtf32(text)))
            {
                const auto screenToWorld = worldToScreen.inverse();

                const Float2 p0 = screenToWorld.transformPoint(Float3{}).xz();
                const Float2 px = screenToWorld.transformPoint(Float3{10, 0, 0}).xz();
                const Float2 pz = screenToWorld.transformPoint(Float3{0, 10, 0}).xz();
                const Float2 axisX = (px - p0).normalized();
                const Float2 axisZ = (pz - p0).normalized();

                const Float2 dragAmount = Mouse::Drag(MouseL);
                auto& node = g_editorState->course.nodes[m_activeNodeIndex];
                const Float2 dx = axisX * dragAmount.x * 0.1f;
                const Float2 dz = axisZ * dragAmount.y * 0.1f;

                node.pos.x += dx.x + dz.x;
                node.pos.z += dx.y + dz.y;

                const float wheel = Mouse::Wheel();
                node.pos.y += wheel * 5.0f;
            }
        }

        // -----------------------------------------------

        ImmediateDrawer::Global().draw();
    }

    void buildSegmentsIfNeeded()
    {
        const auto rebuildIndexes = BuildCourseSegmentIfNeeded(m_segments, g_editorState->course.nodes);

        if (m_courseDrawers.size() != m_segments.size())
        {
            m_courseDrawers.resize(m_segments.size());
        }

        for (const auto i : rebuildIndexes)
        {
            const auto modelBuffer = BuildCourseModel(m_segments[i]);
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

            if (ImGui::InputInt(std::format("Roll##{}", i).c_str(), &nodeList[i].roll, 5, 15))
            {
                nodeList[i].roll = std::clamp(nodeList[i].roll, -180, 180);
            }

            int style = static_cast<int>(nodeList[i].style);
            if (ImGui::Combo(std::format("Style##{}", i).c_str(),
                             &style,
                             GetEnumCStrList<CourseSegmentStyle>().data(),
                             static_cast<int>(CourseSegmentStyle::Max)))
            {
                nodeList[i].style = static_cast<CourseSegmentStyle>(style);
            }

            if (ImGui::Button(std::format("Add Gimmick##{}", i).c_str()))
            {
                nodeList[i].gimmicks.push_back({});
            }

            for (int g = 0; g < nodeList[i].gimmicks.size(); ++g)
            {
                int gimmick = static_cast<int>(nodeList[i].gimmicks[g]);
                if (ImGui::Combo(std::format("Gimmick [{}]##{}", g, i).c_str(),
                                 &gimmick,
                                 GetEnumCStrList<CourseGimmickKind>().data(),
                                 static_cast<int>(CourseGimmickKind::Max)))
                {
                    nodeList[i].gimmicks[g] = static_cast<CourseGimmickKind>(gimmick);
                }

                ImGui::SameLine();

                if (ImGui::Button(std::format("Delete [{}]##gimmicks{}", g, i).c_str()))
                {
                    nodeList[i].gimmicks.erase(nodeList[i].gimmicks.begin() + g);
                    break;
                }
            }
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
            auto firstElement = nodeList.empty() ? CourseNode{} : nodeList.front();
            auto lastElement = nodeList.empty() ? CourseNode{} : nodeList.back();
            lastElement.pos = (lastElement.pos + firstElement.pos) * 0.5f;
            nodeList.push_back(lastElement);
        }

        ImGui::SameLine();

        if (ImGui::Button("Insert Node After Active"))
        {
            if (InRange(m_activeNodeIndex, 0, static_cast<int>(nodeList.size() - 1)))
            {
                auto element = nodeList[m_activeNodeIndex];
                auto forwardElement = nodeList[Modulo<int>(m_activeNodeIndex + 1, nodeList.size())];
                element.pos = (element.pos + forwardElement.pos) * 0.5f;
                nodeList.insert(nodeList.begin() + m_activeNodeIndex + 1, element);
                m_activeNodeIndex += 1;
            }
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
