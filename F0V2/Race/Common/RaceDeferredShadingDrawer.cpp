#include "pch.h"
#include "RaceDeferredShadingDrawer.h"

#include "Asset.generated.h"
#include "RaceSharedState.h"
#include "Race/IRaceDrawer.h"
#include "TY/ComputeDispatcher.h"
#include "TY/Graphics3D.h"
#include "TY/Mat4x4.h"
#include "TY/RenderTargetTexture.h"
#include "TY/System.h"
#include "Util/DebugTomlValue.h"

using namespace Race;

namespace
{
    struct DrawerElement
    {
        std::shared_ptr<IRaceDrawer> drawer;
        bool initialized{};
        RaceDrawParameters parameters{};
    };

    struct Scenery_b10
    {
        Mat4x4 g_projectionMatrixInv{};
        Mat4x4 g_viewMatrixInv{};
        Mat4x4 g_worldToShadowProjection{};
        Float2 g_outputResolution{};
        Float2 g_mousePosition{};
        Float2 g_mouseUV{};
        float g_time{};
    };

    class SceneryDrawer
    {
    public:
        void Init(const UnorderedRenderTargetTexture& outputTexture)
        {
            m_dispatcher =
                ComputeDispatcherParams{}
                .setCS(Asset_shader::scenery1_cs)
                .setSamplers({
                    GraphicsSamplerOptions{}
                    .setAddress(GraphicsAddressMode::Border)
                    .setFilter(GraphicsFilterMode::Linear),
                    GraphicsSamplerOptions{}
                    .setFilter(GraphicsFilterMode::Linear)
                    .setComparison(GraphicsComparisonFunction::Greater)
                    .setMaxAnisotropy(1)
                })
                .setCbv({m_cb})
                .setSrv({
                    g_sharedState->gbuffer.albedo,
                    g_sharedState->gbuffer.normal,
                    g_sharedState->gbuffer.viewDistance,
                    g_sharedState->gbufferTarget.getDepthBuffer(),
                    g_sharedState->shadowMap.getFrontRtv(),
                })
                .setUav({outputTexture});
        }

        void Draw(float renderScale)
        {
            m_cb->g_projectionMatrixInv = Graphics3D::ProjectionMatrix().inverse();
            m_cb->g_viewMatrixInv = Graphics3D::ViewMatrix().inverse();
            m_cb->g_worldToShadowProjection = g_sharedState->cb.shadowCaster->g_worldToShadowProjection;
            m_cb->g_outputResolution = g_sharedState->gbufferTarget.size() * renderScale;
            m_cb->g_time = System::Time();
            m_cb.upload();

            const Size rtvSize = g_sharedState->gbufferTarget.size();
            constexpr int threadsPerGroup = 32;
            const int threadGroup = (rtvSize.x * rtvSize.y / 2) / threadsPerGroup;
            m_dispatcher.dispatch(threadGroup);
        }

    private:
        ConstantBufferWrapper<Scenery_b10> m_cb{};
        ComputeDispatcher m_dispatcher{};
    };
}

struct RaceDeferredShadingDrawer::Impl
{
    SceneryDrawer m_sceneryDrawer{};
    UnorderedRenderTargetTexture m_outputTexture{};

    void Init()
    {
        m_outputTexture =
            RenderTargetTextureParams()
            .setSize(g_sharedState->gbufferTarget.size())
            .setClearColor(ColorF32{0.0f, 0.0f});

        m_sceneryDrawer.Init(m_outputTexture);
    }

    void Draw(float renderScale)
    {
#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("draw_scenery"))
#endif
        {
            m_sceneryDrawer.Draw(renderScale);
        }
    }
};

namespace Race
{
    RaceDeferredShadingDrawer::RaceDeferredShadingDrawer() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceDeferredShadingDrawer::init()
    {
        p_impl->Init();
    }

    void RaceDeferredShadingDrawer::draw(float renderScale) const
    {
        p_impl->Draw(renderScale);
    }

    RenderTargetTexture RaceDeferredShadingDrawer::getOutputTexture() const
    {
        return p_impl->m_outputTexture;
    }
}
