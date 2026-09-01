#include "pch.h"
#include "RaceSetupBackgroundDrawer.h"

#include "Asset.generated.h"
#include "TY/ActorContainer.h"
#include "TY/ComputeDispatcher.h"
#include "TY/DynamicBinding.h"
#include "TY/GameTime.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/ModelDrawer.h"
#include "TY/RenderTarget.h"
#include "TY/Screen.h"

using namespace RaceSetup;

namespace
{
    struct Shadertoy_b0
    {
        Float2 g_screenResolution;
        float g_time;
    };
}

struct RaceSetupBackgroundDrawer::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"RaceSetupBackgroundDrawer";
#endif

    ActorContainer m_children{};

    UnorderedRenderTargetTexture m_outputTexture{};
    Shadertoy_b0 m_cb0{};
    ComputeDispatcher m_dispatcher{};

    void Init()
    {
        m_outputTexture = RenderTargetTextureParams().setSize(Screen::Size());

        m_dispatcher =
            ComputeDispatcherParams()
            .setCS(Asset_shader::race_setup_background_cs)
            .setDynamicCbvCount(1)
            .setUav({m_outputTexture});
    }

    void Draw()
    {
        m_cb0.g_screenResolution = Screen::SizeF();
        m_cb0.g_time += InGameDeltaTime() * 0.125f; // FIXME

        DynamicBinding::SetDynamicCbv(m_dispatcher.mapDynamicCbvIndex(0), m_cb0);
        m_dispatcher.dispatch((Screen::Size().x + 7) / 8, (Screen::Size().y + 7) / 8);

        Immediate2D::Texture(m_outputTexture).resized(Screen::Size()).pushAuto();
        ImmediateDrawer::Global().draw();
    }

private:
    void update() override
    {
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace RaceSetup
{
    RaceSetupBackgroundDrawer::RaceSetupBackgroundDrawer() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceSetupBackgroundDrawer::init()
    {
        p_impl->Init();
    }

    void RaceSetupBackgroundDrawer::draw() const
    {
        p_impl->Draw();
    }

    std::shared_ptr<ActorBase> RaceSetupBackgroundDrawer::asActor() const
    {
        return p_impl;
    }
}
