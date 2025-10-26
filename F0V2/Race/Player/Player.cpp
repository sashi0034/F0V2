#include "pch.h"
#include "Player.h"

#include "Asset.generated.h"
#include "Asset0.h"
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
#include "TY/Palette.h"
#include "TY/Scene.h"
#include "TY/Utils.h"
#include "TY_Extension/GameObjectBase.h"
#include "TY_Extension/Pose.h"

using namespace Race;

namespace
{
    bool s_stopMove{};

    bool s_fixedCameraUp{};
}

struct Player::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    MachinePhysicsState m_physicsState{};
    MachinePhysicsProps m_physicsProps{};

    Float3 m_cameraUp{0, 1, 0};

    float m_maxDurability{5000.0f};

    void Init()
    {
        ModelBuffer model = ModelBuffer{
            PrimitiveModel3D::Capsule(m_physicsState.m_radius, m_physicsState.m_height, ColorF32{0.5f, 0.7f, 1.0f})
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
        const Float3 forwardVector = m_physicsState.m_forwardVector;

        outTarget = m_physicsState.m_pose.position + m_physicsState.m_upVector * 5.0f;

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
        static Mat4x4 localRotation = Mat4x4(Quaternion::RotateX(Math::HalfPiF));
        m_drawer.uploadWorldMatrix(localRotation * m_physicsState.m_pose.getMatrix()).draw();

        m_physicsProps.input.accelPressed = KeyLShift.pressed();

        m_physicsProps.input.boostRequested = KeySpace.down();

        m_physicsProps.input.rightHandling =
            (KeyA.pressed() ? -1.0f : 0.0f) + (KeyD.pressed() ? 1.0f : 0.0f);

        m_physicsProps.input.driftTrigger =
            (KeyLeft.pressed() ? -1 : (KeyRight.pressed() ? 1 : 0));

        m_physicsProps.debugPrint = true;

#ifdef _DEBUG
        if (s_stopMove)
        {
            auto previousState = m_physicsState;
            UpdateMachinePhysicsState(m_physicsState, m_physicsProps);
            m_physicsState = previousState;
        }
        else
#endif
        {
            UpdateMachinePhysicsState(m_physicsState, m_physicsProps);
        }

        // -----------------------------------------------
        // 次のフレームの camera を決定 --> [次フレーム] 前フレームの camera 適応 & 前フレームの Player 描画

        Float3 eyePos, targetPos;
        computeEyeAndTarget(eyePos, targetPos);

        for (const float dt : StandardStep_60Hz())
        {
            m_cameraUp = m_cameraUp.slerp(m_physicsState.m_upVector, dt * 5.0f);
        }

#ifdef _DEBUG
        if (s_fixedCameraUp) m_cameraUp = Float3{0, 1, 0};
#endif

        GetRaceContextContent().camera.set(eyePos, targetPos, m_cameraUp);

        // -----------------------------------------------

        drawUI();

        debugUI();
    }

    void resetPhysicsState()
    {
        m_physicsState = {};

        m_physicsState.m_pose.position = GetRaceContext().stageManager().courseSegments()[0].p1 + Float3{0, 5, 0};

        m_physicsState.m_durability = m_maxDurability;
    }

    void resetPhysicsProps()
    {
        m_physicsProps.targetVelocity = 100.0f;

        m_physicsProps.accelFactor = 1.0f;
    }

    void debugUI()
    {
        ImGui::Begin("Player");

        ImGui::Checkbox("Stop Move", &s_stopMove);

        ImGui::DragFloat3("Position", &m_physicsState.m_pose.position.x, 0.1f);

        if (ImGui::CollapsingHeader("Checkpoint Teleport"))
        {
            static int s_checkpointIndex{};
            ImGui::InputInt("Checkpoint Index", &s_checkpointIndex);

            const auto& segments = GetRaceContext().stageManager().courseSegments();
            s_checkpointIndex = Math::Clamp<int>(s_checkpointIndex, 0, static_cast<int>(segments.size() - 1));

            if (ImGui::Button("Go To Checkpoint"))
            {
                const auto& s = segments[s_checkpointIndex];
                m_physicsState = {};
                m_physicsState.m_pose.position = s.p1 + s.midwayStrips[0].normal * 10.0f;

                s_stopMove = false;
            }
        }

        // -----------------------------------------------

        ImGui::Separator();

        static std::deque<MachinePhysicsState> s_physicsHistory{};
        static int s_rewindFrames{};

        if (not s_stopMove)
        {
            s_physicsHistory.push_back(m_physicsState);
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
            m_physicsState = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
            m_physicsState.m_velocity = {};
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            s_stopMove = false;
        }

        if (ImGui::InputInt("(Rewind Frames9", &s_rewindFrames))
        {
            s_rewindFrames = std::clamp(s_rewindFrames, 0, static_cast<int>(s_physicsHistory.size()) - 1);
            s_stopMove = true;
            m_physicsState = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
            m_physicsState.m_velocity = {};
        }

        // -----------------------------------------------

        ImGui::Checkbox("Fix Camera Up", &s_fixedCameraUp);

        const auto& surfaceNormal = m_physicsState.m_surfaceNormal;
        ImGui::Text("Normal: (%.3f, %.3f, %.3f)", surfaceNormal.x, surfaceNormal.y, surfaceNormal.z);

        ImGui::Separator();

        ImGui::DragFloat("Max Velocity", &m_physicsProps.targetVelocity);

        ImGui::DragFloat("Acceleration Rate", &m_physicsProps.accelFactor, 0.01f);

        if (ImGui::Button("Reset Physics State"))
        {
            resetPhysicsState();
        }

        ImGui::End();
    }

    void drawUI() const
    {
        // スピードメーター
        Immediate2D_Text::ZXProto_Sdf(ToUtf32(std::format("{:.1f} km/h", m_physicsState.m_velocity.length() * 10.0f)))
            .setPosition(Scene::SizeF().movedBy(-20.0f, -12.0f), Alignment9::BottomRight)
            .setSize(28.0f)
            .pushAuto();

        // -----------------------------------------------
        // 耐久値バー
        {
            const float barRate = Math::Clamp(m_physicsState.m_durability / m_maxDurability, 0.0f, 1.0f);
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
            Immediate2D_Text::RocknRoll_Sdf(ToUtf32("{}", static_cast<int>(m_physicsState.m_durability)))
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

    std::shared_ptr<GameObjectBase> Player::asGameObject() const
    {
        return p_impl;
    }
}
