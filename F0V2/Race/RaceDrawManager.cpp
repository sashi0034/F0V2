#include "pch.h"
#include "RaceDrawManager.h"

#define A_CPU
#include "asset/shader/fsr1/ffx_a.h"
#include "asset/shader/fsr1/ffx_fsr1.h"

#include "Asset.generated.h"
#include "IRaceContext.h"
#include "IRaceDrawer.h"
#include "RaceContextContent.h"
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

        RenderTargetTexture GetOutputTarget() const
        {
            return m_outputTexture;
        }

    private:
        ConstantBufferWrapper<Scenery_b10> m_cb{};
        UnorderedRenderTargetTexture m_outputTexture{};
        ComputeDispatcher m_dispatcher{};
    };

    struct CheapAA_b10
    {
        Float2 g_outputResolution;
    };

    // TODO: 使わない
    struct CheapAABuffer : IGenericModelBuffer
    {
        GenericModelShapeBufferElement m_shape{};

        CheapAABuffer()
        {
            m_shape.materialIndex = 0;
            m_shape.indexBuffer = IndexBuffer::Placeholder(6);
        }

        int shapeCount() const override
        {
            return 1; // Assuming a single shape
        }

        GenericModelShapeBufferElement shapeAt(int index) const override
        {
            return m_shape;
        }

        int materialCount() const override
        {
            return 1; // Assuming a single material for the shape
        }

        ConstantBufferArrayImpl materialCbv() const override
        {
            return {Empty};
        }

        Array<Array<ShaderResourceType>> materialSrv() const override
        {
            return {};
        }
    };

    struct EasuCB
    {
        std::array<uint32_t, 4> Const0{};
        std::array<uint32_t, 4> Const1{};
        std::array<uint32_t, 4> Const2{};
        std::array<uint32_t, 4> Const3{};
    };

    struct RcasCB
    {
        std::array<uint32_t, 4> Const0{};
    };

    class Fsr1Upscaler
    {
    public:
        void Init(const TextureHandle& input, const Size& outputSize)
        {
            m_outputSize = outputSize;

            m_easuTexture =
                RenderTargetTextureParams()
                .setSize(outputSize)
                .setFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);

            m_rcasTexture =
                RenderTargetTextureParams()
                .setSize(outputSize)
                .setFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);

            m_easuDispatcher = ComputeDispatcher{
                ComputeDispatcherParams{}
                .setCS(Asset_shader::fsr1_easu_cs)
                .setCbv({m_easuCB})
                .setSrv({input})
                .setUav({m_easuTexture})
            };

            m_rcasDispatcher = ComputeDispatcher{
                ComputeDispatcherParams{}
                .setCS(Asset_shader::fsr1_rcas_cs)
                .setCbv({m_rcasCB})
                .setSrv({m_easuTexture})
                .setUav({m_rcasTexture})
            };
        }

        void Dispatch(const SizeF& inputSize, float sharpnessAttenuation)
        {
            // EASU
            {
                EasuCB cb{};
                FsrEasuCon(reinterpret_cast<AU1*>(&cb.Const0),
                           reinterpret_cast<AU1*>(&cb.Const1),
                           reinterpret_cast<AU1*>(&cb.Const2),
                           reinterpret_cast<AU1*>(&cb.Const3),
                           static_cast<AF1>(inputSize.x),
                           static_cast<AF1>(inputSize.y),
                           static_cast<AF1>(m_outputSize.x),
                           static_cast<AF1>(m_outputSize.y),
                           static_cast<AF1>(m_outputSize.x),
                           static_cast<AF1>(m_outputSize.y));
                m_easuCB.uploadValue(cb);

                int groupsX = (Screen::Size().x + 15) / 16;
                int groupsY = (Screen::Size().y + 15) / 16;
                m_easuDispatcher.dispatch(groupsX, groupsY, 1);
            }

            // RCAS
            {
                RcasCB cb{};
                FsrRcasCon(reinterpret_cast<AU1*>(&cb.Const0), sharpnessAttenuation);
                m_rcasCB.uploadValue(cb);

                int groupsX = (Screen::Size().x + 15) / 16;
                int groupsY = (Screen::Size().y + 15) / 16;
                m_rcasDispatcher.dispatch(groupsX, groupsY, 1);
            }
        }

        TextureHandle OutputTexture() const
        {
            return m_rcasTexture;
        }

    private:
        // Size m_inputSize{};
        Size m_outputSize{};

        UnorderedRenderTargetTexture m_easuTexture{};
        ConstantBufferWrapper<EasuCB> m_easuCB;
        ComputeDispatcher m_easuDispatcher{};

        UnorderedRenderTargetTexture m_rcasTexture{};
        ConstantBufferWrapper<RcasCB> m_rcasCB;
        ComputeDispatcher m_rcasDispatcher{};
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

    GenericModelDrawer m_aaDrawer{};
    ConstantBufferWrapper<CheapAA_b10> m_aaCB10{};
    RenderTarget m_aaTarget{};

    Fsr1Upscaler m_fsr1Upscaler{};

    void Init()
    {
        m_sceneryDrawer.Init();

        m_aaDrawer =
            GenericModelDrawerParams{}
            .setModel(std::make_unique<CheapAABuffer>())
            .setVertexInput({})
            .setShader(Asset_shader::cheap_aa)
            .setOptions(GraphicsOptions{})
            .setCbv10AndLater({m_aaCB10})
            .setSrv10AndLater({m_sceneryDrawer.GetOutputTarget()});

        m_aaTarget =
            RenderTargetParams{}
            .setRtv(RtvParams{}
                    .setFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
                    .setSize(g_sharedState->gbufferTarget.size())
                    .setClearColor(ColorF32{0.0f, 0.0f}));

        m_fsr1Upscaler.Init(m_aaTarget.getFrontRtv(), Screen::Size());
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
                auto projectionMat = Mat4x4::PerspectiveFov(
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

            auto bind = g_sharedState->shadowMap.scopedBind();

            for (int i = 0; i < m_drawers.size(); ++i)
            {
                m_drawers[i].drawer->drawShadowMap();
            }
        }

        float renderScale = 0.4f;

        // GBuffer パス
        {
            g_sharedState->gbufferTarget.setViewport(RectF{Screen::SizeF() * renderScale});
            auto bind = g_sharedState->gbufferTarget.scopedBind();

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
            m_sceneryDrawer.Draw(renderScale);
        }

        // AA
        {
            m_aaCB10->g_outputResolution = g_sharedState->gbufferTarget.size() * renderScale;
            m_aaCB10.upload();

            const auto bind = m_aaTarget.scopedBind();
            m_aaDrawer.draw();
        }

#if 0
        Immediate2D::Texture(m_aaTarget.getFrontRtv())
            .trimmed(RectF{Screen::SizeF() * renderScale})
            .resized(Screen::Size())
            .pushAuto();
        ImmediateDrawer::Global().draw();
#else
        // FSR
        {
            constexpr float k_sharpnessAttenuation = 0.0f;
            m_fsr1Upscaler.Dispatch(Screen::SizeF() * renderScale, k_sharpnessAttenuation);

            Immediate2D::Texture(m_fsr1Upscaler.OutputTexture()).pushAuto();
            ImmediateDrawer::Global().draw();
        }
#endif

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
