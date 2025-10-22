#include "pch.h"
#include "EditorNodeTool.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "ColorPalette.h"
#include "Util/DebugUI.h"
#include "EditorState.h"
#include "Race/Common/CourseModelBuilder.h"
#include "Race/Common/CourseData.h"
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
        auto& nodeList = g_editorState->course.nodes;
        Array<int> rebuildIndexes{};
        for (int i = 0; i < nodeList.size(); ++i)
        {
            int i0 = Modulo<int>(i - 1, nodeList.size());
            int i1 = i;
            int i2 = Modulo<int>(i + 1, nodeList.size());
            int i3 = Modulo<int>(i + 2, nodeList.size());

            const Float3& p0 = nodeList[i0].pos;
            const Float3& p1 = nodeList[i1].pos;
            const Float3& p2 = nodeList[i2].pos;
            const Float3& p3 = nodeList[i3].pos;

            const float p0_roll = nodeList[i0].rollRadians();
            const float p1_roll = nodeList[i1].rollRadians();
            const float p2_roll = nodeList[i2].rollRadians();
            const float p3_roll = nodeList[i3].rollRadians();

            const auto style = nodeList[i1].style;

            if (i >= m_segments.size() ||
                m_segments[i].side_p0 != p0 ||
                m_segments[i].p1 != p1 ||
                m_segments[i].p2 != p2 ||
                m_segments[i].side_p3 != p3 ||
                m_segments[i].side_p0_roll != p0_roll ||
                m_segments[i].p1_roll != p1_roll ||
                m_segments[i].p2_roll != p2_roll ||
                m_segments[i].side_p3_roll != p3_roll ||
                m_segments[i].style != style)
            {
                if (i >= m_segments.size())
                {
                    m_segments.push_back({});
                }

                auto& segment = m_segments[i];
                segment.side_p0_roll = p0_roll;
                segment.p1 = p1;
                segment.p2 = p2;
                segment.side_p3_roll = p3_roll;

                segment.side_p0 = p0;
                segment.p1_roll = p1_roll;
                segment.p2_roll = p2_roll;
                segment.side_p3 = p3;

                segment.style = style;

                rebuildIndexes.push_back(i0);
                rebuildIndexes.push_back(i1);
                rebuildIndexes.push_back(i2);
                rebuildIndexes.push_back(i3);
            }
        }

        std::ranges::sort(rebuildIndexes);

        // 重複を除去
        {
            auto last = std::ranges::unique(rebuildIndexes);
            rebuildIndexes.erase(last.begin(), last.end());
        }

        while (m_segments.size() > nodeList.size())
        {
            m_segments.pop_back();
        }

        // -----------------------------------------------

        // 変更があった CourseSegment の線分に対して面を構築する
        for (const auto i : rebuildIndexes)
        {
            const auto& priorSegment = m_segments[Modulo<int>(i - 1, m_segments.size())];
            const auto& nextSegment = m_segments[(i + 1) % m_segments.size()];

            auto& segment = m_segments[i];
            segment.midwayStrips.clear();

            const int samplesPerSegment = (segment.p2 - segment.p1).length() / 5.0f;
            const auto midwayPositions = GenerateCatmullRomPoints(
                segment.side_p0, segment.p1, segment.p2, segment.side_p3, samplesPerSegment);
            const auto midwayRolls = GenerateCatmullRomAngles(
                segment.side_p0_roll, segment.p1_roll, segment.p2_roll, segment.side_p3_roll, samplesPerSegment);

            for (int m = 0; m < midwayPositions.size() - 1 /* 終端は除外 */; ++m)
            {
                CourseStrip strip{};
                strip.center = midwayPositions[m];

                const float roll = midwayRolls[m];

                auto nextPosition = midwayPositions[m + 1];
                strip.toNext = nextPosition - strip.center;
                strip.lengthToNext = strip.toNext.length();

                {
                    const Float3 n = strip.toNext.cross(Float3(0, 1, 0));
                    strip.normal = n.cross(strip.toNext); // 鉛直上ベクトルと進行方向に垂直なベクトル

                    const auto q = Quaternion(strip.toNext.normalized(), roll);
                    strip.normal = q.rotate(strip.normal).normalized();
                }

                auto right = strip.toNext.cross(strip.normal).normalized();
                float width = 12.5f; // TODO
                strip.leftmost = strip.center - right * width;
                strip.rightmost = strip.center + right * width;

                strip.style = segment.style;

                if (strip.style == CourseSegmentStyle::Pipe)
                {
                    if (priorSegment.style != CourseSegmentStyle::Pipe &&
                        m < PipeEntryExitStrips)
                    {
                        // 入口
                        strip.style = CourseSegmentStyle::Road;
                    }
                    else if (nextSegment.style != CourseSegmentStyle::Pipe &&
                        m >= midwayPositions.size() - PipeEntryExitStrips)
                    {
                        // 出口
                        strip.style = CourseSegmentStyle::Road;
                    }
                }

                if (strip.style == CourseSegmentStyle::Pipe)
                {
                    // トンネル頂点の計算
                    for (int t = 0; t < PipeSubdivision; ++t)
                    {
                        // 円周上の方向ベクトルを計算
                        const float angle =
                            Math::HalfPiF - (0.5 + static_cast<float>(t) / PipeSubdivision) * Math::TwoPi_v<float>;
                        const Float3 dir = Quaternion(strip.toNext.normalized(), angle).rotate(strip.normal);
                        strip.pipe.ringVectors[t] = dir;
                    }
                }

                segment.midwayStrips.push_back(strip);
            }

            // 終端部分は次のセクションで行う
        }

        if (m_courseDrawers.size() != m_segments.size())
        {
            m_courseDrawers.resize(m_segments.size());
        }

        for (const auto i : rebuildIndexes)
        {
            auto& segment = m_segments[i];

            // 終端部分の追加
            {
                // TODO: バグ修正
                auto& segment1 = m_segments[(i + 1) % m_segments.size()];
                segment.midwayStrips.push_back(segment1.midwayStrips[0]);
            }

            segment.totalLength = 0.0f;
            for (int m = 0; m < segment.midwayStrips.size(); ++m)
            {
                segment.totalLength += segment.midwayStrips[m].lengthToNext;
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

            if (ImGui::CollapsingHeader(std::format("Gimmicks##{}", i).c_str()))
            {
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

                if (ImGui::Button(std::format("Add Gimmick##{}", i).c_str()))
                {
                    nodeList[i].gimmicks.push_back(CourseGimmickKind::BoostPad_C);
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
