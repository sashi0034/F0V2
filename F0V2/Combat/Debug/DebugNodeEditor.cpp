#include "pch.h"
#include "DebugNodeEditor.h"

#include "Asset.generated.h"
#include "DebugEditorState.h"
#include "TY/ActorContainer.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
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
        m_drawer.uploadWorldMatrix(Mat4x4::Identity()).draw();
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
