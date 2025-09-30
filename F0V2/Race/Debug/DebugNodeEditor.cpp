#include "pch.h"
#include "DebugNodeEditor.h"

#include "Asset.generated.h"
#include "DebugEditorState.h"
#include "TY/ActorContainer.h"
#include "TY/ModelDrawer.h"
#include "TY/Periodic.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/Shape2D.h"
#include "TY/Shape3D.h"
#include "TY/ShapeDrawer.h"
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

    struct CourseSegment
    {
        Float3 p1{};
        Float3 p2{};

        Array<Float3> interpolatedPoints{};
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

        for (int i = 0; i < m_segments.size(); ++i)
        {
            const auto& pos = m_segments[i].p1;
            m_drawer.uploadWorldMatrix(Mat4x4::Translate(pos)).draw();
        }

        Shape3D::LineSet lineSet{};
        for (int i = 0; i < m_segments.size(); ++i)
        {
            const auto& segment = m_segments[i];
            for (int j = 0; j < segment.interpolatedPoints.size() - 1; ++j)
            {
                lineSet.appendLine(segment.interpolatedPoints[j], segment.interpolatedPoints[j + 1]);
            }
        }

        lineSet.setColor(ColorF32{1.0f, 0.5f, 0.1f})
               .pushAuto();

        ShapeDrawer::Global().draw();
    }

    void buildSegmentsIfNeeded()
    {
        auto& nodeList = g_debugEditorState->course.nodes;
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
                segment.interpolatedPoints = generateCatmullRomPoints(p0, p1, p2, p3, 10);
            }
        }

        while (m_segments.size() > nodeList.size())
        {
            m_segments.pop_back();
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
