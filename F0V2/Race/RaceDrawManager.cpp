#include "pch.h"
#include "RaceDrawManager.h"

#include "Asset.generated.h"
#include "IRaceContext.h"
#include "IRaceDrawer.h"
#include "RaceContextContent.h"
#include "RaceDrawQualityController.h"
#include "RaceDrawUpscaler.h"
#include "Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/ComputeDispatcher.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Graphics3D.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Logger.h"
#include "TY/ModelDrawer.h"
#include "TY/RenderTarget.h"
#include "TY/RenderTargetTexture.h"
#include "TY/Screen.h"
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
        void Init()
        {
            m_outputTexture =
                RenderTargetTextureParams()
                .setSize(g_sharedState->gbufferTarget.size())
                .setClearColor(ColorF32{0.0f, 0.0f});

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
                .setUav({m_outputTexture});
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

        RenderTargetTexture GetOutputTexture() const
        {
            return m_outputTexture;
        }

    private:
        ConstantBufferWrapper<Scenery_b10> m_cb{};
        UnorderedRenderTargetTexture m_outputTexture{};
        ComputeDispatcher m_dispatcher{};
    };
}

struct RaceDrawManager::Impl : ActorBase
{
#if defined(_DEBUG)
    std::string m_debugName{"RaceDrawManager"};
#endif

    ActorContainer m_children{};

    Array<DrawerElement> m_drawers{};

    SceneryDrawer m_sceneryDrawer{};

    RenderTarget m_transparentDrawTarget{};

    RaceDrawQualityController m_qualityController{};

    RaceDrawUpscaler m_drawUpscaler{};

    void Init()
    {
        m_sceneryDrawer.Init();

        m_transparentDrawTarget =
            RenderTargetParams()
            .setRtv(m_sceneryDrawer.GetOutputTexture())
            .setDepthBuffer(g_sharedState->gbufferTarget.getDepthBuffer());

        m_drawUpscaler.init(m_sceneryDrawer.GetOutputTexture());
    }

    void Unregister(const IRaceDrawer* drawer)
    {
        for (int i = 0; i < m_drawers.size(); ++i)
        {
            if (m_drawers[i].drawer.get() == drawer)
            {
                m_drawers.erase(m_drawers.begin() + i);
                return;
            }
        }

        LogError("RaceDrawManager::Unregister(): Drawer {} not found.", static_cast<const void*>(drawer));
    }

private:
    void update() override
    {
        // カメラ行列適応
        {
            Graphics3D::SetViewMatrix(GetRaceContextContent().camera.viewMatrix());

            {
                const auto projectionMat = Mat4x4::PerspectiveFov(
                    g_sharedState->fov,
                    Screen::Size().horizontalAspectRatio(),
                    g_sharedState->nearDepth,
                    g_sharedState->farDepth
                );

                Graphics3D::SetProjectionMatrix(projectionMat);
            }
        }

        for (int i = 0; i < m_drawers.size(); ++i)
        {
            m_drawers[i].drawer->prepareDrawParameters(m_drawers[i].parameters, not m_drawers[i].initialized);
            m_drawers[i].initialized = true;
        }

        // Shadow パス
        {
            updateShadowMapMatrix();

            const auto bind = g_sharedState->shadowMap.scopedClearBind();

            for (int i = 0; i < m_drawers.size(); ++i)
            {
                m_drawers[i].drawer->drawShadowMap();
            }
        }

        m_qualityController.update();
        const auto qualityTarget = m_qualityController.getQualityData();

        // GBuffer パス
        {
            g_sharedState->gbufferTarget.setViewport(RectF{Screen::SizeF() * qualityTarget.renderScale});
            const auto bind = g_sharedState->gbufferTarget.scopedClearBind();

            for (int i = 0; i < m_drawers.size(); ++i)
            {
                m_drawers[i].drawer->drawGBuffer();
            }
        }

        // レイマーチング
#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("draw_scenery"))
#endif
        {
            m_sceneryDrawer.Draw(qualityTarget.renderScale);
        }

        // 半透明描画パス
        // TODO: 半透明オブジェクトはリニア色空間で描画するべきかも? 要調査
        {
            m_transparentDrawTarget.setViewport(RectF{Screen::SizeF() * qualityTarget.renderScale});
            const auto bind = m_transparentDrawTarget.scopedBind();

            for (int i = 0; i < m_drawers.size(); ++i)
            {
                m_drawers[i].drawer->drawTransparent();
            }
        }

        // アップスケーリング
        {
            Immediate2D::Texture output =
                m_drawUpscaler.upscale(qualityTarget.renderScale, qualityTarget.fsrEnabled);
            output.pushAuto();
            ImmediateDrawer::Global().draw();
        }

        for (int i = 0; i < m_drawers.size(); ++i)
        {
            m_drawers[i].drawer->drawUI();
        }
    }

    void updateShadowMapMatrix()
    {
        const Mat4x4 cameraMatrix = GetRaceContextContent().camera.worldMatrix();
        const Float3 cameraForward = cameraMatrix.forward();
        const Float3 cameraRight = cameraMatrix.right();
        const Float3 cameraUp = cameraMatrix.up();

        const Float3 cameraEye = GetRaceContextContent().camera.eyePosition();

        // カメラ方向を光源とする
        const Float3 lightDirection = -(cameraUp - cameraForward * 0.5f).normalized();
        const Float3& lightRight = cameraRight;
        // -GetRaceContextContent().camera.upDirection();

        const Float3 shadowCenter = cameraEye;

        const Float3 shadowEyePosition = shadowCenter - lightDirection * 200.0f;

        // 平行投影
        constexpr float orthoSize = 50.0f;
        constexpr float nearDepth = 0.1f;
        constexpr float farDepth = 500.0f;
        const auto shadowProjection = Mat4x4::Orthographic(
            orthoSize * 2.0f, // width
            orthoSize * 2.0f, // height
            nearDepth,
            farDepth
        );

        const auto shadowView = Mat4x4::LookAt(
            shadowEyePosition,
            shadowCenter,
            lightDirection.cross(lightRight).normalized()
        );

        const auto shadowViewProjection = shadowView * shadowProjection;

        //                 /|                ---
        //                / |                 |
        //               /  |                 |
        //              /   |                 | farHalfH 
        //             /|   |  ---            |
        //            / |   |   | nearHalfH   |
        // cameraEye |--|---|  ---           ---
        //            \ |   |
        //             \|   |
        //              \   |
        //               \  |
        //                \ |
        //                 \|

        const float cameraFov = g_sharedState->fov;
        const float cameraNearDepth = g_sharedState->nearDepth;

        // NOTE: カスケード化する場合はこの値を調節する
        constexpr float cropFarDepth = orthoSize; // g_sharedState->farDepth;

        const float nearHalfH = tanf(cameraFov / 2.0f) * cameraNearDepth;
        const float nearHalfW = nearHalfH * Screen::Size().horizontalAspectRatio();

        const float farHalfH = tanf(cameraFov / 2.0f) * cropFarDepth;
        const float farHalfW = farHalfH * Screen::Size().horizontalAspectRatio();

        const Float3 nearCenter = cameraEye + cameraForward * cameraNearDepth;
        const Float3 farCenter = cameraEye + cameraForward * cropFarDepth;

        std::array<Float3, 8> frustumCorners{};
        frustumCorners[0] = nearCenter + cameraRight * nearHalfW - cameraUp * nearHalfH;
        frustumCorners[1] = nearCenter + cameraRight * nearHalfW + cameraUp * nearHalfH;
        frustumCorners[2] = nearCenter - cameraRight * nearHalfW + cameraUp * nearHalfH;
        frustumCorners[3] = nearCenter - cameraRight * nearHalfW - cameraUp * nearHalfH;
        frustumCorners[4] = farCenter + cameraRight * farHalfW - cameraUp * farHalfH;
        frustumCorners[5] = farCenter + cameraRight * farHalfW + cameraUp * farHalfH;
        frustumCorners[6] = farCenter - cameraRight * farHalfW + cameraUp * farHalfH;
        frustumCorners[7] = farCenter - cameraRight * farHalfW - cameraUp * farHalfH;

        Float3 minP{FLT_MAX, FLT_MAX, FLT_MAX};
        Float3 maxP{-FLT_MAX, -FLT_MAX, -FLT_MAX};
        for (auto& corner : frustumCorners)
        {
            corner = shadowViewProjection.transformPoint(corner);
            minP = MinVector3(minP, corner);
            maxP = MaxVector3(maxP, corner);
        }

        // クロップ行列を作成
        Float2 scaling = Float2{2.0f, 2.0f} / (maxP.xy() - minP.xy());
        Float2 translation = -(minP.xy() + maxP.xy()) * Float2{0.5f, 0.5f} * scaling;
        Mat4x4 cropMatrix = Mat4x4::Identity();
        cropMatrix.mat.r[0].m128_f32[0] = scaling.x;
        cropMatrix.mat.r[1].m128_f32[1] = scaling.y;
        cropMatrix.mat.r[3].m128_f32[0] = translation.x;
        cropMatrix.mat.r[3].m128_f32[1] = translation.y;

        g_sharedState->cb.shadowCaster->g_worldToShadowProjection = shadowViewProjection * cropMatrix;
        g_sharedState->cb.shadowCaster.upload();
    }

    float orderPriority() const override
    {
        return -1000.0f;
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Race
{
    RaceDrawManager::RaceDrawManager() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceDrawManager::init()
    {
        p_impl->Init();
    }

    void RaceDrawManager::registerDrawer(const std::shared_ptr<IRaceDrawer>& drawer)
    {
        assert(drawer != nullptr);
        p_impl->m_drawers.push_back(DrawerElement{drawer, {}});
    }

    void RaceDrawManager::unregisterDrawer(const IRaceDrawer* drawer)
    {
        assert(drawer != nullptr);
        p_impl->Unregister(drawer);
    }

    std::shared_ptr<ActorBase> RaceDrawManager::asActor() const
    {
        return p_impl;
    }
}
