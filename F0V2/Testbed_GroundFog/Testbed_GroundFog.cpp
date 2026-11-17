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
    constexpr Size FogTextureSize{512, 512};
    constexpr float CameraInteractionDistance = 1.0f;
    constexpr float CameraInteractionDecay = 1.5f;

    struct alignas(16) FogAdvectionParams_b0
    {
        Float2 textureResolution{};
        float deltaTime{};
        float terrainSize{};
    };

    struct alignas(16) ColumnUpdateParams_b0
    {
        float terrainSize{};
        float padding[3]{};
    };

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

        const Float3 cameraPosition = m_camera.eyePosition();
        m_lastCameraInteractionPosition = Float2{cameraPosition.x, cameraPosition.z};
        m_cameraInteractionPosition = m_lastCameraInteractionPosition;

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

        const auto fogTextureParams = RenderTargetTextureParams{}
            .setSize(FogTextureSize)
            .setClearColor(ColorF32{0.0f, 0.0f, 1.0f, 0.0f})
            .setFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);

        // Fog Texture は RG = ノイズ座標オフセット、B = 濃度、A = かき乱され具合とする。
        for (auto& fogTexture : m_fogTextures)
        {
            fogTexture = UnorderedRenderTargetTexture{fogTextureParams};

            // 2 枚とも (0, 0, 1, 0) で初期化する。
            const RenderTarget clearTarget{
                RenderTargetParams{}
                .setRtv(static_cast<const RenderTargetTexture&>(fogTexture))
            };
            const auto clearScope = clearTarget.scopedClearBind();
        }

        // 移流結果を反映した Column Texture を毎フレームここへ書き込む。
        m_updatedColumnTexture = UnorderedRenderTargetTexture{
            RenderTargetTextureParams{}
            .setSize(FogTextureSize)
            .setClearColor(ColorF32{0.0f, 0.0f})
            .setFormat(DXGI_FORMAT_R32G32_FLOAT)
        };

        const TextureHandle fogTextureHandles[] = {
            static_cast<const RenderTargetTexture&>(m_fogTextures[0]),
            static_cast<const RenderTargetTexture&>(m_fogTextures[1]),
        };

        const auto linearClampSampler = GraphicsSamplerOptions{}
            .setAddress(GraphicsAddressMode::Clamp)
            .setFilter(GraphicsFilterMode::Linear);
        const auto linearWrapSampler = GraphicsSamplerOptions{}
            .setAddress(GraphicsAddressMode::Wrap)
            .setFilter(GraphicsFilterMode::Linear);

        // 読み元と書き先が逆になる 2 通りの Dispatcher を用意する。
        for (int sourceIndex = 0; sourceIndex < 2; ++sourceIndex)
        {
            const int destinationIndex = 1 - sourceIndex;
            m_fogAdvectionDispatchers[sourceIndex] = ComputeDispatcher{
                ComputeDispatcherParams{}
                .setCS(ComputeShader{ShaderParams::CS("asset/shader/fog_advection_cs.hlsl")})
                .setSamplers({linearClampSampler})
                .setSrv({
                    fogTextureHandles[sourceIndex],
                    m_terrain.GetVelocityFieldTexture()
                })
                .setUav({static_cast<UnorderedTextureHandle>(m_fogTextures[destinationIndex])})
                .setDynamicCbvCount(1)
            };

            m_columnUpdateDispatchers[sourceIndex] = ComputeDispatcher{
                ComputeDispatcherParams{}
                .setCS(ComputeShader{ShaderParams::CS("asset/shader/column_update_cs.hlsl")})
                .setSamplers({linearWrapSampler})
                .setSrv({
                    m_terrain.GetColumnNoiseTexture(),
                    fogTextureHandles[sourceIndex]
                })
                .setUav({m_updatedColumnTexture})
                .setDynamicCbvCount(1)
            };
        }

        m_deferredDispatcher = ComputeDispatcher{
            ComputeDispatcherParams{}
            .setCS(ComputeShader{ShaderParams::CS("asset/shader/ground_fog_deferred_cs.hlsl")})
            .setSamplers({linearClampSampler})
            .setSrv({
                m_gbufferColor,
                m_gbufferNormal,
                m_gbuffer.getDepthBuffer(),
                m_terrain.GetHeightMapTexture(),
                static_cast<const RenderTargetTexture&>(m_updatedColumnTexture)
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

        const float deltaTime = static_cast<float>(System::DeltaTime());
        m_elapsedTime += deltaTime;

        // カメラ位置を XZ 平面へ真下に投影する。
        const Float3 cameraPosition = m_camera.eyePosition();
        const Float2 cameraGroundPosition{cameraPosition.x, cameraPosition.z};
        const Float2 cameraDisplacement = cameraGroundPosition - m_lastCameraInteractionPosition;
        const float cameraDistance = cameraDisplacement.length();

        m_cameraInteractionStrength *= std::exp(-CameraInteractionDecay * deltaTime);
        if (cameraDistance >= CameraInteractionDistance)
        {
            m_cameraInteractionPosition = cameraGroundPosition;
            m_cameraInteractionDirection = cameraDisplacement / cameraDistance;
            m_cameraInteractionStrength = 1.0f;
            m_lastCameraInteractionPosition = cameraGroundPosition;
        }

        // Simplex Noise から Curl Noise を計算し、速度場を毎フレーム更新する。
        m_terrain.UpdateVelocityField(
            m_elapsedTime,
            m_cameraInteractionPosition,
            m_cameraInteractionDirection,
            m_cameraInteractionStrength);

        // Semi-Lagrangian 法で Fog Texture を移流し、読み書きを入れ替える。
        const FogAdvectionParams_b0 fogAdvectionParams{
            .textureResolution = FogTextureSize.cast<Float2>(),
            .deltaTime = deltaTime,
            .terrainSize = Terrain::TerrainSize,
        };
        DynamicBinding::SetDynamicCbv(0, fogAdvectionParams);
        m_fogAdvectionDispatchers[m_currentFogTextureIndex].dispatch(
            (FogTextureSize.x + 7) / 8,
            (FogTextureSize.y + 7) / 8);
        m_currentFogTextureIndex = 1 - m_currentFogTextureIndex;

        // 移流後の Fog Texture から Column Texture を毎フレーム更新する。
        const ColumnUpdateParams_b0 columnUpdateParams{
            .terrainSize = Terrain::TerrainSize,
        };
        DynamicBinding::SetDynamicCbv(0, columnUpdateParams);
        m_columnUpdateDispatchers[m_currentFogTextureIndex].dispatch(
            (FogTextureSize.x + 7) / 8,
            (FogTextureSize.y + 7) / 8);

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
    std::array<UnorderedRenderTargetTexture, 2> m_fogTextures{};
    UnorderedRenderTargetTexture m_updatedColumnTexture{};
    std::array<ComputeDispatcher, 2> m_fogAdvectionDispatchers{};
    std::array<ComputeDispatcher, 2> m_columnUpdateDispatchers{};
    ComputeDispatcher m_deferredDispatcher{};
    int m_currentFogTextureIndex{};
    float m_elapsedTime{};
    Float2 m_lastCameraInteractionPosition{};
    Float2 m_cameraInteractionPosition{};
    Float2 m_cameraInteractionDirection{1.0f, 0.0f};
    float m_cameraInteractionStrength{};
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
