#include "pch.h"
#include "Player.h"

#include "Asset.generated.h"
#include "GM/DebugService.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Common/RaceSharedState.h"
#include "Race/Machine/MachineDrawer.h"
#include "Race/Machine/MachinePhysics.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/Gamepad.h"
#include "TY/GameTime.h"
#include "TY/KeyboardInput.h"
#include "TY/Mouse.h"
#include "TY/Palette.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/SimpleInput.h"
#include "TY_Extension/GameObjectBase.h"
#include "TY_Extension/Pose.h"
#include "Util/DebugTomlValue.h"
#include "Util/DoubleTapDetector.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

namespace
{
#if defined(_DEBUG)
    bool s_stopMove{};
#endif
}

struct Player::Impl : GameObjectBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
    ActorContainer m_children{};

    MachineDrawer m_drawer{};

    MachineId m_machineId{PlayerMachineId};

    float m_previousAttackedByOtherMachineTime{};

    Util::DoubleTapDetector m_rightHyperTurnDetector{};
    Util::DoubleTapDetector m_leftHyperTurnDetector{};

    void Init()
    {
        GetRaceContext().registerDrawer(shared_from_this());

        constexpr auto color = Palette::CornflowerBlue;
        machine().props.themeColor = color;
        m_drawer.init(PlayerMachineId, color.sRGBToLinear());

        resetPhysicsProps();
        resetPhysicsState();

        // TODO: Update 前の最初のフレームでカメラが設定されるようにする
    }

private:
    MachinePhysicsUnit& machine() const
    {
        return GetRaceContext().machineManager().fetchMachine(PlayerMachineId);
    }

    void update() override
    {
        updatePhysics();

        m_drawer.update();

        debugUI();
    }

    void drawShadowMap() const override
    {
        m_drawer.drawShadowMap();
    }

    void drawGBuffer() const override
    {
        m_drawer.drawGBuffer();
    }

    void drawTransparent() const override
    {
        m_drawer.drawTransparent();
    }

    void resetPhysicsState()
    {
        SetupMachinePhysicsState(machine().state, machine().props);
    }

    void resetPhysicsProps()
    {
        machine().props.machineId = 0;

        machine().props.peakVelocity = 100.0f;

        machine().props.accelFactor = 1.0f;
    }

    void updatePhysics()
    {
        const MachinePhysicsProps::input_t previousInput = machine().props.input;

        MachinePhysicsProps::input_t input;

        bool leftHyperTurn{}, rightHyperTurn{};
        if (IsUsingGamepad())
        {
            input.accelPressed = MainGamepad.a().pressed ||
                (machine().state.isBoostUnlocked() && MainGamepad.b().pressed);

            input.boostRequested = MainGamepad.b().down;

            input.rightHandling = MainGamepad.axisL().x;

            input.pitch = MainGamepad.axisL().y;

            input.driftTrigger = -MainGamepad.leftTrigger() + MainGamepad.rightTrigger();

            leftHyperTurn = input.rightHandling < -0.1f && MainGamepad.lb().down;
            rightHyperTurn = input.rightHandling > 0.1f && MainGamepad.rb().down;
        }
        else
        {
            input.accelPressed = KeyLShift.pressed();

            input.boostRequested = KeySpace.down();

            input.rightHandling =
                (KeyA.pressed() ? -1.0f : 0.0f) + (KeyD.pressed() ? 1.0f : 0.0f);

            input.pitch =
                (KeyW.pressed() ? -1.0f : (KeyS.pressed() ? 1.0f : 0.0f));

            input.driftTrigger =
                (KeyLeft.pressed() ? -1.0f : (KeyRight.pressed() ? 1.0f : 0.0f));
        }

        // ハイパーターン入力処理 (ダブルタップ)
        leftHyperTurn |=
            m_leftHyperTurnDetector.update(input.rightHandling < -0.1f && input.driftTrigger < -TriggerButtonThreshold);
        rightHyperTurn |=
            m_rightHyperTurnDetector.update(input.rightHandling > 0.1f && input.driftTrigger > TriggerButtonThreshold);

        input.hyperTurnRequested = leftHyperTurn || rightHyperTurn;

#if defined(_DEBUG)
        if (g_debugService.disablePlayerInput)
        {
            input = {};
        }
#endif

        machine().props.input = input;

#if defined(_DEBUG)
        if (s_stopMove)
        {
            auto previousState = machine().state;
            UpdateMachinePhysicsState(machine().state, machine().props);
            machine().state = previousState;
        }
        else
#endif
        {
            const auto updateOutcome = UpdateMachinePhysicsState(machine().state, machine().props);
            GetRaceContext().machineManager().eventHandler().handleIfNeeded(machine().id());

            playSoundIfNeeded(updateOutcome);
        }

#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("player_immortal"))
        {
            machine().state.m_durability = machine().props.maxDurability;
        }
#endif
    }

    void playSoundIfNeeded(const MachinePhysicsUpdateOutcome updateOutcome)
    {
        const auto& machineState = machine().state;

        // -----------------------------------------------
        // 操作入力の効果音

        if (updateOutcome.accelInputAccepted)
        {
            if (not Asset_sound::Accel().isPlayingUnique())
            {
                Asset_sound::Accel().setLoopEnabled(true);
                Asset_sound::Accel().playUnique(2.0f); // TODO: 音量
            }
        }
        else
        {
            Asset_sound::Accel().stopUnique();
        }

        if (updateOutcome.driftInputAccepted)
        {
            if (not Asset_sound::Drift().isPlayingUnique())
            {
                Asset_sound::Drift().setLoopEnabled(true);
                Asset_sound::Drift().playUnique(0.5f);
            }
        }
        else
        {
            Asset_sound::Drift().stopUnique();
        }

        if (updateOutcome.boostInputAccepted)
        {
            Asset_sound::Boost().playOneShot();
        }

        // -----------------------------------------------
        // ギミック接触の効果音

        const GimmickFlagBits newTouchingGimmicks =
            machineState.m_touchingGimmicks & (~machineState.m_previousTouchingGimmicks);

        if (machineState.m_touchingGimmicks & GimmickFlag::Barrier)
        {
            Asset_sound::Collide().playOneShot();
        }

        if (newTouchingGimmicks & GimmickFlag::BoostPad)
        {
            Asset_sound::Boost().playOneShot();
        }

        if (newTouchingGimmicks & GimmickFlag::JumpPad)
        {
            Asset_sound::JumpPad().playOneShot();
        }

        if (machineState.m_touchingGimmicks & GimmickFlag::PitZone)
        {
            if (not Asset_sound::RecoverPad().isPlayingUnique())
            {
                Asset_sound::RecoverPad().setLoopEnabled(true);
                Asset_sound::RecoverPad().playUnique();
            }
        }
        else
        {
            Asset_sound::RecoverPad().stopUnique();
        }

        // -----------------------------------------------

        if (machineState.m_isFallingOffCourse)
        {
            Asset_sound::DeathUp().playUnique();
        }
        else
        {
            Asset_sound::DeathUp().stopUnique();
        }

        if (m_previousAttackedByOtherMachineTime != machineState.m_lastAttackedByOtherMachineTime)
        {
            m_previousAttackedByOtherMachineTime = machineState.m_lastAttackedByOtherMachineTime;
            Asset_sound::Attacked().playOneShot();
        }
    }

    void debugUI()
    {
#if defined(_DEBUG)
        ImGui::Begin("Player");

        ImGui::Checkbox("Stop Move", &s_stopMove);

        ImGui::DragFloat3("Position", &machine().state.m_pose.position.x, 0.1f);

        // -----------------------------------------------
        ImGui::Separator();

        // if (ImGui::CollapsingHeader("Checkpoint Teleport"))
        {
            static int s_checkpointIndex{};
            ImGui::InputInt("Checkpoint Index", &s_checkpointIndex);

            const auto& segments = GetRaceContext().stageManager().courseSegments();
            s_checkpointIndex = Math::Clamp<int>(s_checkpointIndex, 0, static_cast<int>(segments.size() - 1));

            if (ImGui::Button("Go To Checkpoint"))
            {
                const auto& s = segments[s_checkpointIndex];
                resetPhysicsState();
                machine().state.m_pose.position = s.p1 + s.midwayStrips[0].normal * 10.0f;

                s_stopMove = false;
            }
        }

        // -----------------------------------------------
        ImGui::Separator();

        static std::deque<MachinePhysicsState> s_physicsHistory{};
        static int s_rewindFrames{};

        if (not s_stopMove)
        {
            s_physicsHistory.push_back(machine().state);
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
            machine().state = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
            machine().state.m_velocity = {};
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            s_stopMove = false;
        }

        if (ImGui::InputInt("(Rewind Frames)", &s_rewindFrames))
        {
            s_rewindFrames = std::clamp(s_rewindFrames, 0, static_cast<int>(s_physicsHistory.size()) - 1);
            s_stopMove = true;
            machine().state = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
            machine().state.m_velocity = {};
        }

        // -----------------------------------------------

        const auto& surfaceNormal = machine().state.m_surfaceNormal;
        ImGui::Text("Normal: (%.3f, %.3f, %.3f)", surfaceNormal.x, surfaceNormal.y, surfaceNormal.z);

        ImGui::Separator();

        ImGui::DragFloat("Max Velocity", &machine().props.peakVelocity);

        ImGui::DragFloat("Acceleration Rate", &machine().props.accelFactor, 0.01f);

        if (ImGui::Button("Reset Physics State"))
        {
            resetPhysicsState();
        }

        ImGui::End();
#endif
    }

    void killed() override
    {
        m_children.killEach();

        GetRaceContext().unregisterDrawer(this);
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

    std::shared_ptr<GameObjectBase> Player::asGameObject() const
    {
        return p_impl;
    }
}
