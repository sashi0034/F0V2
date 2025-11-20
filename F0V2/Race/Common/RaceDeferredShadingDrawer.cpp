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
        void Init(
            const UnorderedRenderTargetTexture& outputTexture,
            const ConstantBufferWrapper<Scenery_b10>& cb)
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
                .setCbv({cb})
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
            const Size rtvSize = g_sharedState->gbufferTarget.size() * renderScale;
            constexpr int threadsPerGroup = 32;
            const int threadGroup = (rtvSize.x * rtvSize.y / 2) / threadsPerGroup;
            m_dispatcher.dispatch(threadGroup);
        }

    private:
        ComputeDispatcher m_dispatcher{};
    };

#if defined(_DEBUG)
    class SimpleDeferredDrawer
    {
    public:
        void Init(
            const UnorderedRenderTargetTexture& outputTexture,
            const ConstantBufferWrapper<Scenery_b10>& cb)
        {
            // TODO: 使ってないリソースの整理
            m_dispatcher =
                ComputeDispatcherParams{}
                .setCS(Asset_shader::simple_deferred_cs)
                .setSamplers({
                    GraphicsSamplerOptions{}
                    .setAddress(GraphicsAddressMode::Border)
                    .setFilter(GraphicsFilterMode::Linear),
                    GraphicsSamplerOptions{}
                    .setFilter(GraphicsFilterMode::Linear)
                    .setComparison(GraphicsComparisonFunction::Greater)
                    .setMaxAnisotropy(1)
                })
                .setCbv({cb})
                .setSrv({
                    g_sharedState->gbuffer.albedo,
                    g_sharedState->gbuffer.normal,
                    g_sharedState->gbuffer.viewDistance,
                    g_sharedState->gbufferTarget.getDepthBuffer(),
                })
                .setUav({outputTexture});
        }

        void Draw(float renderScale)
        {
            const Size rtvSize = g_sharedState->gbufferTarget.size() * renderScale;
            m_dispatcher.dispatch(rtvSize.x / 4, rtvSize.y / 8);
        }

    private:
        ComputeDispatcher m_dispatcher{};
    };
#endif
}

struct RaceDeferredShadingDrawer::Impl
{
    UnorderedRenderTargetTexture m_outputTexture{};

    ConstantBufferWrapper<Scenery_b10> m_cb{};

    SceneryDrawer m_sceneryDrawer{};

#if defined(_DEBUG)
    SimpleDeferredDrawer m_debugDrawer{};
#endif

    void Init()
    {
        m_outputTexture =
            RenderTargetTextureParams()
            .setSize(g_sharedState->gbufferTarget.size())
            .setClearColor(ColorF32{0.0f, 0.0f});

        m_sceneryDrawer.Init(m_outputTexture, m_cb);

#if defined(_DEBUG)
        m_debugDrawer.Init(m_outputTexture, m_cb);
#endif
    }

    void Draw(float renderScale)
    {
        m_cb->g_projectionMatrixInv = Graphics3D::ProjectionMatrix().inverse();
        m_cb->g_viewMatrixInv = Graphics3D::ViewMatrix().inverse();
        m_cb->g_worldToShadowProjection = g_sharedState->cb.shadowCaster->g_worldToShadowProjection;
        m_cb->g_outputResolution = g_sharedState->gbufferTarget.size() * renderScale;
        m_cb->g_time = System::Time();
        m_cb.upload();

#if defined(_DEBUG)
        if (not GetDebugTomlValue<bool>("draw_scenery"))
        {
            m_debugDrawer.Draw(renderScale);
        }
        else
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
