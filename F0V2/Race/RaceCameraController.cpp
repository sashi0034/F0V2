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

struct RaceCameraController::Impl
{
    Float3 m_cameraUp{0, 1, 0};

    void Update()
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
        const auto& machine = GetRaceContext().getMachine(machineId);

        // -----------------------------------------------

        for (const float dt : StandardStep_60Hz())
        {
            m_cameraUp = m_cameraUp.slerp(machine.state.m_upVector, dt * 5.0f); // TODO: 2.0f などもを試して調整
        }

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

private:
    void computeEyeAndTarget(const MachinePhysicsUnit& machine, Float3& outEye, Float3& outTarget) const
    {
        const Float3 forwardVector = machine.state.m_forwardVector;

        outTarget = machine.state.m_pose.position + machine.state.m_upVector * 5.0f;

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

    void debugUI()
    {
        ImGui::Begin("Camera Controller");

        ImGui::Checkbox("Use Debug Camera", &s_useDebugCamera);

        ImGui::Checkbox("Fix Camera Up", &s_fixedCameraUp);

        ImGui::End();
    }
};

namespace Race
{
    RaceCameraController::RaceCameraController() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceCameraController::update()
    {
        p_impl->Update();
    }
}
