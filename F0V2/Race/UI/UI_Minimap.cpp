#include "pch.h"
#include "UI_Minimap.h"

#include "Asset.generated.h"
#include "Race/IRaceContext.h"
#include "Race/Machine/MachineConstants.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/Graphics3D.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Mat4x4.h"
#include "TY/Math.h"
#include "TY/ModelDrawer.h"
#include "TY/Palette.h"
#include "TY/RenderTarget.h"
#include "TY/Screen.h"

using namespace Race;

namespace
{
    constexpr Size MinimapTextureSize{256, 256};

    // NOTE: 正射影ではこの値を変えても見た目の拡大率は変わらず、クリップ範囲だけが動く。
    constexpr float CameraHeight = 50.0f;
    constexpr float CameraNearZ = 1.0f;
    constexpr float CameraFarZ = 100.0f; // CameraHeight より十分大きくしないと、プレイヤーより下の地形が far 側で切り落とされる

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

    RenderTarget m_renderTarget{};

    Array<ModelDrawer> m_courseMinimapDrawer{};

    void Init()
    {
        m_renderTarget =
            RenderTargetParams{}
            .setRtv(
                RtvParams{}
                .setSize(MinimapTextureSize)
                .setClearColor(ColorF32{0.0f, 0.0f})); // アルファ 0 で透明クリア

        const auto& courseModels = GetRaceContext().stageManager().courseModels();

        m_courseMinimapDrawer.clear();
        for (const auto& model : courseModels)
        {
            // NOTE: StageManager はモデルを持たないセグメントにも空要素を入れてくる
            if (model.isEmpty()) continue;

            m_courseMinimapDrawer.push_back(
                ModelDrawerParams{}
                .setModel(model)
                .setShader(Asset_shader::minimap)
                .setOptions(GraphicsOptions::FromTarget(m_renderTarget)));
        }
    }

    void Draw() const
    {
        // レンダーターゲットを差し替える前に、積まれているスクリーン向けの図形を吐き出しておく
        ImmediateDrawer::Global().draw();

        const auto& machines = GetRaceContext().machineManager().machineList();
        const auto& player = machines[PlayerMachineId];

        const Float3 playerPos = player.state.m_pose.position;
        const Float3 playerUp = player.state.m_upVector;
        const auto& playerLocation = player.state.m_lapProgress.segmentAndStrip();
        const Float3 roadForward = GetRaceContext().stageManager().courseSegments()[playerLocation.segmentIndex]
            .midwayStrips[playerLocation.stripIndex].toNext.normalized(); // TODO: lerp

        // プレイヤーの頭上から、進行方向が画面上向きになるように見下ろす
        const Mat4x4 view = Mat4x4::LookAt(playerPos + playerUp * CameraHeight, playerPos, roadForward);

        // 代案: Mat4x4::PerspectiveFov(Math::ToRadians(50.0f), 1.0f, CameraNearZ, CameraFarZ)
        constexpr float CameraViewSize = 300.0f; // 正射影で切り取るワールド空間の一辺の長さ
        const Mat4x4 projection = Mat4x4::Orthographic(CameraViewSize, CameraViewSize, CameraNearZ, CameraFarZ);

        // -----------------------------------------------

        const Mat4x4 previousView = Graphics3D::ViewMatrix();
        const Mat4x4 previousProjection = Graphics3D::ProjectionMatrix();

        Graphics3D::SetViewMatrix(view);
        Graphics3D::SetProjectionMatrix(projection);
        {
            const auto bind = m_renderTarget.scopedClearBind();

            // コース (頂点はワールド空間に焼かれているのでワールド行列は単位行列)
            for (const auto& drawer : m_courseMinimapDrawer)
            {
                drawer.setWorldMatrix(Mat4x4::Identity()).draw();
            }

            drawMachineMarkers(machines, view * projection);

            // ここまでに積んだ図形をミニマップのテクスチャへ流す
            ImmediateDrawer::Global().draw();
        }

        Graphics3D::SetViewMatrix(previousView);
        Graphics3D::SetProjectionMatrix(previousProjection);

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

        Float2 markerPos{};
        for (const auto& machine : machines)
        {
            if (machine.id() == PlayerMachineId) continue;
            if (not projectToTexture(machine.state.m_pose.position, markerPos)) continue;

            constexpr float RivalMarkerRadius = 4.0f;
            pushMachineMarker(markerPos, RivalMarkerRadius, Palette::Crimson);
        }

        const auto& player = machines[PlayerMachineId];
        if (projectToTexture(player.state.m_pose.position, markerPos))
        {
            constexpr float PlayerMarkerRadius = 4.0f;
            pushMachineMarker(markerPos, PlayerMarkerRadius, Palette::CornflowerBlue);
        }
    }

    void update() override
    {
        m_children.updateEach();
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
