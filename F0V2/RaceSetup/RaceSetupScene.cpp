#include "pch.h"
#include "RaceSetupScene.h"

#include "Asset.generated.h"
#include "TY/ActorContainer.h"
#include "TY/Graphics3D.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/ModelDrawer.h"
#include "TY/Screen.h"

using namespace RaceSetup;

namespace
{
}

struct RaceSetupScene::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"RaceSetupScene";
#endif

    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    void Init()
    {
        m_drawer =
            ModelDrawerParams{}
            .setModel(Asset_model::cinnamon)
            .setShader(Asset_shader::model);
    }

private:
    void update() override
    {
        Immediate2D::Rect{Screen::RectF().stretched(-12.0f)}
            .setColor(ColorF32{0.1f, 0.1f, 0.1f})
            .pushAuto();
        ImmediateDrawer::Global().draw();

        const Mat4x4 viewMat = Mat4x4::LookAt(Float3{0, 0, 5}, Float3{0, 0, 0}, Float3{0, 1, 0});

        const Mat4x4 projectionMat = Mat4x4::PerspectiveFov(
            90.0_deg,
            Screen::Size().horizontalAspectRatio(),
            1.0f,
            10.0f
        );

        Graphics3D::SetViewMatrix(viewMat);
        Graphics3D::SetProjectionMatrix(projectionMat);

        m_drawer.uploadWorldMatrix(Mat4x4::Identity()).draw();
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace RaceSetup
{
    RaceSetupScene::RaceSetupScene() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceSetupScene::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> RaceSetupScene::asActor() const
    {
        return p_impl;
    }
}
