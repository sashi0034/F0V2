#include "pch.h"
#include "Testbed_GroundFog.h"

#include "Terrain.h"

#include "TY/ComputeDispatcher.h"
#include "TY/DynamicBinding.h"
#include "TY/Gamepad.h"
#include "TY/Graphics3D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Mat4x4.h"
#include "TY/RenderTarget.h"
#include "TY/RenderTargetTexture.h"
#include "TY/Screen.h"
#include "TY/Shader.h"
#include "TY/SimpleCamera3D.h"
#include "TY/System.h"
#include "TY/Window.h"

using namespace TY;

namespace
{
    constexpr float FarZ = 500.0f;
    constexpr Size RenderSize{1920, 1080};

    struct alignas(16) DeferredParams_b0
    {
        Mat4x4 projectionMatrixInv{};
        Mat4x4 viewMatrixInv{};
        Float2 outputResolution{};
        Float2 padding{};
    };
}

class Testbed_GroundFog_impl
{
public:
    Testbed_GroundFog_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));
        m_camera.reset(Float3{0.0f, 24.0f, -32.0f}, Float3{0.0f, 2.0f, 0.0f});

        // GBuffer はカラーと法線、RenderTarget が生成するデプスバッファを使用する。
        m_gbufferColor = RenderTargetTexture{
            RenderTargetTextureParams{}
            .setSize(RenderSize)
            .setClearColor(ColorF32{0.0f, 0.0f})
        };
        m_gbufferNormal = RenderTargetTexture{
            RenderTargetTextureParams{}
            .setSize(RenderSize)
            .setClearColor(ColorF32{0.5f, 0.5f, 1.0f, 0.0f})
        };
        m_gbuffer = RenderTarget{
            RenderTargetParams{}
            .setRtvList({m_gbufferColor, m_gbufferNormal})
        };

        m_deferredOutput = UnorderedRenderTargetTexture{
            RenderTargetTextureParams{}
            .setSize(RenderSize)
            .setClearColor(ColorF32{0.0f, 0.0f})
        };

        m_deferredDispatcher = ComputeDispatcher{
            ComputeDispatcherParams{}
            .setCS(ComputeShader{ShaderParams::CS("asset/shader/ground_fog_deferred_cs.hlsl")})
            .setSamplers({
                GraphicsSamplerOptions{}
                .setAddress(GraphicsAddressMode::Clamp)
                .setFilter(GraphicsFilterMode::Linear)
            })
            .setSrv({
                m_gbufferColor,
                m_gbufferNormal,
                m_gbuffer.getDepthBuffer(),
                m_terrain.GetHeightMapTexture(),
                m_terrain.GetColumnTexture()
            })
            .setUav({m_deferredOutput})
            .setDynamicCbvCount(1)
        };
    }

    void Update()
    {
        m_camera.transformBySimpleInput();

        Graphics3D::SetViewMatrix(m_camera.viewMatrix());
        Graphics3D::SetProjectionMatrix(Mat4x4::PerspectiveFov(
            75.0_deg,
            RenderSize.horizontalAspectRatio(),
            0.1f,
            FarZ));

        // カラーと法線の GBuffer、デプスバッファへ地形を描画する。
        {
            const auto gbufferScope = m_gbuffer.scopedClearBind();
            m_terrain.Draw();
        }

        // GBuffer とデプスをコンピュートシェーダーで統合する。
        const DeferredParams_b0 deferredParams{
            .projectionMatrixInv = Graphics3D::ProjectionMatrix().inverse(),
            .viewMatrixInv = Graphics3D::ViewMatrix().inverse(),
            .outputResolution = RenderSize.cast<Float2>(),
        };
        DynamicBinding::SetDynamicCbv(0, deferredParams);
        m_deferredDispatcher.dispatch(
            (RenderSize.x + 7) / 8,
            (RenderSize.y + 7) / 8);

        auto& immediateDrawer = ImmediateDrawer::Global();
        const TextureHandle deferredOutput = static_cast<const RenderTargetTexture&>(m_deferredOutput);
        immediateDrawer.push(
            Immediate2D::Texture{deferredOutput}
            .resized(RenderSize.cast<Float2>())
            .setPosition(Float2{0.0f, 0.0f}));

#if 1
        // ハイトマップのデバッグ表示
        immediateDrawer.push(
            Immediate2D::Texture{m_terrain.GetHeightMapDebugTexture()}
            .resized(Float2{256.0f, 256.0f})
            .setPosition(Float2{16.0f, 16.0f}));

        // 速度場テクスチャをそのまま可視化する。
        immediateDrawer.push(
            Immediate2D::Texture{m_terrain.GetVelocityFieldTexture()}
            .resized(Float2{256.0f, 256.0f})
            .setPosition(Float2{288.0f, 16.0f}));
#endif

        immediateDrawer.draw();
    }

private:
    SimpleCamera3D m_camera{};
    Terrain m_terrain{};
    RenderTargetTexture m_gbufferColor{};
    RenderTargetTexture m_gbufferNormal{};
    RenderTarget m_gbuffer{};
    UnorderedRenderTargetTexture m_deferredOutput{};
    ComputeDispatcher m_deferredDispatcher{};
};

void Testbed_GroundFog()
{
    Testbed_GroundFog_impl impl{};

    Window::SetTitle("Testbed_GroundFog");

    Screen::RequestResize(RenderSize);

    while (System::Update())
    {
        impl.Update();
    }
}
