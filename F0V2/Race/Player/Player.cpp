#include "pch.h"
#include "Player.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "GM/DebugService.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Machine/MachinePhysics.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/GameStep.h"
#include "TY/GameTime.h"
#include "TY/KeyboardInput.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Mouse.h"
#include "TY/Palette.h"
#include "TY/Scene.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"
#include "TY/Utils.h"
#include "TY_Extension/GameObjectBase.h"
#include "TY_Extension/Pose.h"

using namespace Race;

namespace
{
#if defined(_DEBUG)
    bool s_stopMove{};
#endif
}

struct Player::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    MachinePhysicsUnit m_machine{};

    Float3 m_cameraUp{0, 1, 0};

    void Init()
    {
        ModelBuffer model = ModelBuffer{
            PrimitiveModel3D::Capsule(m_machine.state.m_radius, m_machine.state.m_height, Palette::CornflowerBlue)
        };

        m_drawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({GetRaceContextContent().cb.lambert});

        resetPhysicsState();
        resetPhysicsProps();

        // TODO: Update 前の最初のフレームでカメラが設定されるようにする
    }

private:
    void computeEyeAndTarget(Float3& outEye, Float3& outTarget) const
    {
        const Float3 forwardVector = m_machine.state.m_forwardVector;

        outTarget = m_machine.state.m_pose.position + m_machine.state.m_upVector * 5.0f;

        constexpr float cameraBackward = 10.0f;
        constexpr float cameraHeight = 5.0f;

        const Float3 optimalEyePos =
            outTarget - forwardVector.normalized() * cameraBackward + m_cameraUp * cameraHeight;

        const auto ray = LineSegment3D{outTarget, optimalEyePos};
        const auto hit =
            GetRaceContext().stageManager().stageStaticCollider().rayCastGround(ray);
        if (hit.has_value())
        {
            // 地面にカメラが遮られているなら、その面に垂線の足をおろしてカメラ位置とする
            const Float3 H = hit->triangle.asPlane().projection(optimalEyePos);
            outEye = H;
            return;
        }

        outEye = optimalEyePos;
    }

    void update() override
    {
        // 前フレームの camera & 前フレームの Player 描画方式
        static Mat4x4 localRotation = Mat4x4(Quaternion::RotateX(Math::HalfPiF));
        m_drawer.uploadWorldMatrix(localRotation * m_machine.state.m_pose.getMatrix()).draw();

        updatePhysics();

        drawUI();

        debugUI();
    }

    void resetPhysicsState()
    {
        m_machine.state = {};

        m_machine.state.m_pose.position = GetRaceContext().stageManager().startPosition();

        m_machine.state.m_durability = m_machine.props.maxDurability;
    }

    void resetPhysicsProps()
    {
        m_machine.props.machineId = 0;

        m_machine.props.peakVelocity = 100.0f;

        m_machine.props.accelFactor = 1.0f;
    }

    void updatePhysics()
    {
        MachinePhysicsProps::input_t input;

        input.accelPressed = KeyLShift.pressed();

        input.boostRequested = KeySpace.down();

        input.rightHandling =
            (KeyA.pressed() ? -1.0f : 0.0f) + (KeyD.pressed() ? 1.0f : 0.0f);

        input.driftTrigger =
            (KeyLeft.pressed() ? -1 : (KeyRight.pressed() ? 1 : 0));

#if defined(_DEBUG)
        if (g_debugService.disablePlayerInput)
        {
            input = {};
        }
#endif

        m_machine.props.input = input;

#if defined(_DEBUG)
        if (s_stopMove)
        {
            auto previousState = m_machine.state;
            UpdateMachinePhysicsState(m_machine.state, m_machine.props);
            m_machine.state = previousState;
        }
        else
#endif
        {
            UpdateMachinePhysicsState(m_machine.state, m_machine.props);
        }
    }

    void debugUI()
    {
        ImGui::Begin("Player");

        ImGui::Checkbox("Stop Move", &s_stopMove);

        ImGui::DragFloat3("Position", &m_machine.state.m_pose.position.x, 0.1f);

        if (ImGui::CollapsingHeader("Checkpoint Teleport"))
        {
            static int s_checkpointIndex{};
            ImGui::InputInt("Checkpoint Index", &s_checkpointIndex);

            const auto& segments = GetRaceContext().stageManager().courseSegments();
            s_checkpointIndex = Math::Clamp<int>(s_checkpointIndex, 0, static_cast<int>(segments.size() - 1));

            if (ImGui::Button("Go To Checkpoint"))
            {
                const auto& s = segments[s_checkpointIndex];
                m_machine.state = {};
                m_machine.state.m_pose.position = s.p1 + s.midwayStrips[0].normal * 10.0f;

                s_stopMove = false;
            }
        }

        // -----------------------------------------------

        ImGui::Separator();

        static std::deque<MachinePhysicsState> s_physicsHistory{};
        static int s_rewindFrames{};

        if (not s_stopMove)
        {
            s_physicsHistory.push_back(m_machine.state);
            s_rewindFrames = 0;
        }

        while (s_physicsHistory.size() > 300)
        {
            s_physicsHistory.pop_front();
        }

        if (ImGui::SliderInt("Rewind Frames", &s_rewindFrames, static_cast<int>(s_physicsHistory.size()) - 1, 0))
        {
            s_rewindFrames = std::clamp(s_rewindFrames, 0, static_cast<int>(s_physicsHistory.size()) - 1);
            s_stopMove = true;
            m_machine.state = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
            m_machine.state.m_velocity = {};
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            s_stopMove = false;
        }

        if (ImGui::InputInt("(Rewind Frames9", &s_rewindFrames))
        {
            s_rewindFrames = std::clamp(s_rewindFrames, 0, static_cast<int>(s_physicsHistory.size()) - 1);
            s_stopMove = true;
            m_machine.state = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
            m_machine.state.m_velocity = {};
        }

        // -----------------------------------------------

        const auto& surfaceNormal = m_machine.state.m_surfaceNormal;
        ImGui::Text("Normal: (%.3f, %.3f, %.3f)", surfaceNormal.x, surfaceNormal.y, surfaceNormal.z);

        ImGui::Separator();

        ImGui::DragFloat("Max Velocity", &m_machine.props.peakVelocity);

        ImGui::DragFloat("Acceleration Rate", &m_machine.props.accelFactor, 0.01f);

        if (ImGui::Button("Reset Physics State"))
        {
            resetPhysicsState();
        }

        ImGui::End();
    }

    void drawUI() const
    {
        // スピードメーター
        Immediate2D_Text::ZXProto_Sdf(ToUtf32(std::format("{:.1f} km/h", m_machine.state.m_velocity.length() * 10.0f)))
            .setPosition(Scene::SizeF().movedBy(-20.0f, -12.0f), Alignment9::BottomRight)
            .setSize(28.0f)
            .pushAuto();

        // -----------------------------------------------
        // 耐久値バー
        {
            const float barRate = Math::Clamp(m_machine.state.m_durability / m_machine.props.maxDurability, 0.0f, 1.0f);
            const Float2 bottomLeft = Scene::RectF().bl().movedBy(40.0f, -160.0f);
            constexpr SizeF barSize{320.0f, 12.0f};
            Immediate2D::RoundRect{RectF{bottomLeft, Alignment9::BottomLeft, barSize}}
                .setColor(ColorF32{0.1f})
                .pushAuto();
            Immediate2D::RoundRect{
                    RectF{bottomLeft, Alignment9::BottomLeft, barSize.withX(barSize.x * barRate)}.stretched(-0.5f)
                }
                .setColor(Palette::GoldenRod)
                .pushAuto();
            Immediate2D_Text::RocknRoll_Sdf(ToUtf32("{}", static_cast<int>(m_machine.state.m_durability)))
                .setSize(20.0f)
                .setPosition(bottomLeft.movedBy(barSize.x, -barSize.y - 4.0f), Alignment9::BottomRight)
                .setColor(Palette::LightSteelBlue)
                .pushAuto();
        }

        // -----------------------------------------------

        ImmediateDrawer::Global().draw();
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"Player";
    }
};

namespace Race
{
    Player::Player() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void Player::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    const MachinePhysicsUnit& Player::machine() const
    {
        return p_impl->m_machine;
    }

    std::shared_ptr<GameObjectBase> Player::asGameObject() const
    {
        return p_impl;
    }
}
