#include "pch.h"
#include "RaceCameraController.h"

#include "IRaceContext.h"
#include "RaceContextContent.h"
#include "GM/DebugService.h"
#include "Stage/StageManager.h"
#include "TY/GameStep.h"
#include "TY/KeyboardInput.h"
#include "TY/Mouse.h"
#include "TY/PrimitiveTypes3D.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"

using namespace Race;

namespace
{
#if defined(_DEBUG)
    bool s_fixedCameraUp{};

    SimpleCamera3D s_debugCamera{};

    bool s_useDebugCamera{};
#endif
}

struct RaceCameraController::Impl : ActorBase
{
    Float3 m_cameraForward{0, 0, 1}; // TODO: 初期位置設定
    Float3 m_cameraUp{0, 1, 0};

private:
    void update() override
    {
        debugUI();

#if defined(_DEBUG)
        g_debugService.disablePlayerInput = s_useDebugCamera;

        if (s_useDebugCamera)
        {
            Float3 moveVector = SimpleInput::GetPlayerMovement3D() * (KeyShift.pressed() ? 50.0f : 10.0f);
            moveVector *= g_debugService.cameraSpeed;

            const Float2 rotateVector = Mouse::Drag(MouseM) * Float2{1, -1} * 5.0f;
            s_debugCamera.transform(System::DeltaTime(), moveVector, rotateVector);

            GetRaceContextContent().camera.set(
                s_debugCamera.eyePosition(), s_debugCamera.targetPosition(), s_debugCamera.upDirection());
            return;
        }
#endif

        // -----------------------------------------------

        int machineId = 0;
#if defined(_DEBUG)
        machineId = g_debugService.monitorMachineId;
#endif
        const auto& machine = GetRaceContext().machineManager().fetchMachine(machineId);

        // -----------------------------------------------

        m_cameraForward = m_cameraForward.rotatedTowards(
            machine.state.m_forwardVector, 5.0f * InGameDeltaTime(), machine.state.rightVector());
        m_cameraUp = m_cameraUp.rotatedTowards(
            machine.state.m_upVector, 5.0f * InGameDeltaTime(), machine.state.rightVector());

        // for (const float dt : StandardStep_60Hz())
        // {
        //     m_cameraForward = m_cameraForward.slerp(machine.state.m_forwardVector, dt * 10.0f);
        //     m_cameraUp = m_cameraUp.slerp(machine.state.m_upVector, dt * 10.0f);
        // }

        Float3 eyePos, targetPos;
        computeEyeAndTarget(machine, eyePos, targetPos);

#if defined(_DEBUG)
        if (s_fixedCameraUp)
        {
            m_cameraUp = Float3{0, 1, 0};
        }
#endif

        GetRaceContextContent().camera.set(eyePos, targetPos, m_cameraUp);
    }

    void computeEyeAndTarget(const MachinePhysicsUnit& machine, Float3& outEye, Float3& outTarget) const
    {
        const float targetUpLength = 5.0f + 0.5f * machine.state.m_pitchRate; // TODO; 改良

        outTarget = machine.state.m_pose.position + m_cameraUp * targetUpLength;

        constexpr float cameraBackward = 10.0f;
        constexpr float cameraHeight = 5.0f;

        const Float3 optimalEyePos =
            outTarget - m_cameraForward * cameraBackward + m_cameraUp * cameraHeight;

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

    void debugUI()
    {
#if defined(_DEBUG)
        ImGui::Begin("Camera Controller");

        ImGui::Checkbox("Use Debug Camera", &s_useDebugCamera);

        if (ImGui::Button("Reset Debug Camera"))
        {
            s_debugCamera.reset();
        }

        ImGui::Checkbox("Fix Camera Up", &s_fixedCameraUp);

        ImGui::End();
#endif
    }

    float orderPriority() const override
    {
        return -500.0f;
    }

    void killed() override
    {
    }
};

namespace Race
{
    RaceCameraController::RaceCameraController() :
        p_impl(std::make_shared<Impl>())
    {
    }

    std::shared_ptr<ActorBase> RaceCameraController::asActor() const
    {
        return p_impl;
    }
}
