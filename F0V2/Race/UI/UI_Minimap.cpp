#include "pch.h"
#include "UI_Minimap.h"

#include "Asset.generated.h"
#include "Race/IRaceContext.h"
#include "Race/Machine/MachineConstants.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/DynamicBinding.h"
#include "TY/GameTime.h"
#include "TY/GenericModelBufferTemplates.h"
#include "TY/GenericModelDrawer.h"
#include "TY/Graphics3D.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Mat4x4.h"
#include "TY/Math.h"
#include "TY/ModelDrawer.h"
#include "TY/Palette.h"
#include "TY/RenderTarget.h"
#include "TY/Screen.h"
#include "Util/ExpLerp.h"

using namespace Race;
using namespace Util;

namespace
{
    /// @brief minimap.hlsl の cbuffer Minimap : register(b10)
    struct Minimap_b10
    {
        Float3 g_lightDirection{};
        float _padding{};
    };

    constexpr Size MinimapTextureSize{400, 400};

    // NOTE: 正射影ではこの値を変えても見た目の拡大率は変わらず、クリップ範囲だけが動く。
    constexpr float CameraHeight = 200.0f;
    constexpr float CameraNearZ = -400.0f;
    constexpr float CameraFarZ = 800.0f; // CameraHeight より十分大きくしないと、プレイヤーより下の地形が far 側で切り落とされる

    void pushMachineMarker(const Float2& position, float radius, const ColorF32& color)
    {
        // 角丸の半径を一辺の半分にすると正円になる
        Immediate2D::RoundRect{
                RectF{position, Alignment9::MiddleCenter, SizeF{radius * 2.0f, radius * 2.0f}}
            }
            .setRoundness(radius, 8)
            .setColor(color)
            .pushAuto();
    }
}

struct UI_Minimap::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"UI_Minimap";
#endif
    ActorContainer m_children{};

    RenderTarget m_courseTarget{}; // コースだけを描く中間ターゲット (縁取りの入力)

    RenderTarget m_renderTarget{}; // 最終ターゲット

    Array<ModelDrawer> m_courseMinimapDrawer{};

    GenericModelDrawer m_outlineDrawer{};

    Float3 m_cameraForward{};

    void Init()
    {
        const auto rtvParams =
            RtvParams{}
            .setSize(MinimapTextureSize)
            .setClearColor(ColorF32{0.0f, 0.0f}); //  // アルファ 0 で透明クリア

        m_courseTarget = RenderTarget{RenderTargetParams{}.setRtv(rtvParams)};
        m_renderTarget = RenderTarget{RenderTargetParams{}.setRtv(rtvParams)};

        const auto& courseModels = GetRaceContext().stageManager().courseMinimapModels();

        m_courseMinimapDrawer.clear();
        for (const auto& model : courseModels)
        {
            if (model.isEmpty()) continue;

            m_courseMinimapDrawer.push_back(
                ModelDrawerParams{}
                .setModel(model)
                .setShader(Asset_shader::minimap)
                .setOptions(
                    GraphicsOptions::FromTarget(m_courseTarget)
                    // 単面モデルなのでカリング無し
                    .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None)))
                .setDynamicCbvCount(1));
        }

        m_outlineDrawer =
            GenericModelDrawerParams{}
            .setModel(std::make_unique<SingleShapeModelBuffer>(6))
            .setVertexInput({})
            .setShader(Asset_shader::minimap_outline)
            .setOptions(
                GraphicsOptions{}
                .setRtvFormats(m_renderTarget.getRtvFormats())
                // コース色と縁はシェーダー内で合成済みなので、そのまま書き込む
                .setBlend(GraphicsBlendOptions::Opaque()))
            .setSrv10AndLater({m_courseTarget.getFrontRtv()});
    }

    void Update()
    {
        const auto& machines = GetRaceContext().machineManager().machineList();
        const auto& player = machines[PlayerMachineId];

        const Float3 playerPos = player.state.m_pose.position;
        const auto& playerLocation = player.state.m_lapProgress.segmentAndStrip();

        const auto& strip = GetRaceContext().stageManager().courseSegments()[playerLocation.segmentIndex]
            .midwayStrips[playerLocation.stripIndex];

        const Float3 targetForward = strip.toNext.normalized();
        if (m_cameraForward.isZero())
        {
            m_cameraForward = targetForward;
        }
        else
        {
            m_cameraForward = m_cameraForward.slerp(targetForward, FastExpAlpha(0.1f, InGameDeltaTime()));
        }

        const Float3 cameraUp = m_cameraForward.cross(strip.rightmost - strip.leftmost).normalized();

        const Mat4x4 view = Mat4x4::LookAt(playerPos + cameraUp * CameraHeight, playerPos, m_cameraForward);

        // 代案: Mat4x4::PerspectiveFov(Math::ToRadians(50.0f), 1.0f, CameraNearZ, CameraFarZ)
        constexpr float CameraViewSize = 800.0f; // 正射影で切り取るワールド空間の一辺の長さ
        const Mat4x4 projection = Mat4x4::Orthographic(CameraViewSize, CameraViewSize, CameraNearZ, CameraFarZ);

        // -----------------------------------------------

        const Mat4x4 previousView = Graphics3D::ViewMatrix();
        const Mat4x4 previousProjection = Graphics3D::ProjectionMatrix();

        Graphics3D::SetViewMatrix(view);
        Graphics3D::SetProjectionMatrix(projection);

        {
            const auto bind = m_courseTarget.scopedClearBind();

            // 光は常に頭上から差す
            const auto cbv = DynamicBinding::UploadDynamicCbv(Minimap_b10{.g_lightDirection = -cameraUp});

            // コース (頂点はワールド空間に焼かれているのでワールド行列は単位行列)
            for (const auto& drawer : m_courseMinimapDrawer)
            {
                DynamicBinding::SetDynamicCbv(10, cbv);
                drawer.setWorldMatrix(Mat4x4::Identity()).draw();
            }
        }

        {
            const auto bind = m_renderTarget.scopedClearBind();

            // コースの不透明部分に白縁をつける (マーカーを描く前に行うので、マーカーは縁取られない)
            m_outlineDrawer.draw();

            drawMachineMarkers(machines, view * projection);

            // ここまでに積んだ図形をミニマップのテクスチャへ流す
            ImmediateDrawer::Global().draw();
        }

        Graphics3D::SetViewMatrix(previousView);
        Graphics3D::SetProjectionMatrix(previousProjection);
    }

    void Draw() const
    {
        // レンダーターゲットを差し替える前に、積まれているスクリーン向けの図形を吐き出しておく
        ImmediateDrawer::Global().draw();

        // 出来上がったテクスチャをスクリーンへ合成する
        Immediate2D::Texture{m_renderTarget.getFrontRtv()}
            .setPosition(
                Screen::BottomRightF().movedBy(-40.0f, -80.0f),
                Alignment9::BottomRight)
            .pushAuto();
    }

private:
    /// @brief 機体を丸で描く (プレイヤーが一番手前に来るように最後に描く)
    void drawMachineMarkers(const Array<MachinePhysicsUnit>& machines, const Mat4x4& viewProjection) const
    {
        const SizeF textureSize{m_renderTarget.size()};

        const auto projectToTexture = [&](const Float3& worldPos, Float2& out) -> bool
        {
            const Float3 ndc = viewProjection.transformPoint(worldPos);
            if (Abs(ndc.x) > 1.0f || Abs(ndc.y) > 1.0f) return false;
            if (ndc.z < 0.0f || ndc.z > 1.0f) return false;

            // NDC は Y が上向き、2D 座標は Y が下向き
            out = Float2{
                (ndc.x * 0.5f + 0.5f) * textureSize.x,
                (0.5f - ndc.y * 0.5f) * textureSize.y,
            };
            return true;
        };

        const auto& machineManager = GetRaceContext().machineManager();
        const int playerRank = machineManager.getEvaluation(PlayerMachineId).rank;

        Float2 markerPos{};
        for (const auto& machine : machines)
        {
            if (machine.id() == PlayerMachineId) continue;
            if (not projectToTexture(machine.state.m_pose.position, markerPos)) continue;

            // プレイヤーより順位が低い敵は色を変える
            const bool isBehindPlayer = machineManager.getEvaluation(machine.id()).rank > playerRank;
            constexpr float RivalMarkerRadius = 4.0f;
            pushMachineMarker(markerPos, RivalMarkerRadius, isBehindPlayer ? Palette::Red : Palette::DeepPink);
        }

        const auto& player = machines[PlayerMachineId];
        if (projectToTexture(player.state.m_pose.position, markerPos))
        {
            constexpr float PlayerMarkerRadius = 4.0f;
            pushMachineMarker(markerPos, PlayerMarkerRadius, Palette::DodgerBlue);
        }
    }

    void update() override
    {
        m_children.updateEach();

        Update();
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Race
{
    UI_Minimap::UI_Minimap() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void UI_Minimap::init()
    {
        p_impl->Init();
    }

    void UI_Minimap::draw() const
    {
        p_impl->Draw();
    }

    std::shared_ptr<ActorBase> UI_Minimap::asActor() const
    {
        return p_impl;
    }
}
