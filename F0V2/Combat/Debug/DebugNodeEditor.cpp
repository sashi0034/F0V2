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

using namespace Combat;

namespace
{
}

struct DebugNodeEditor::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

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
        auto& nodeList = g_debugEditorState->course.nodes;
        for (int i = 0; i < nodeList.size(); ++i)
        {
            const auto& pos = nodeList[i].pos;
            m_drawer.uploadWorldMatrix(Mat4x4::Translate(pos)).draw();
        }

        for (int i = 0; i < nodeList.size(); ++i)
        {
            const auto& pos = nodeList[i].pos;
            const auto& nextPos = nodeList[(i + 1) % nodeList.size()].pos;
            Shape3D::Line{pos, nextPos}
                .setColor(ColorF32{0.5f, 0.7f, 1.0f}.lerp(ColorF32{0.0f, 1.0f, 0.5f}, Periodic::Sine0_1(1.0s)))
                .pushAuto();
        }

        ShapeDrawer::Global().draw();

        nodeEditor();
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

namespace Combat
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
