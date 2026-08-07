#include "pch.h"
#include "CharacterAILogic.h"

#include "SpatialAI.h"
#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"
#include "TY/GameTime.h"
#include "TY/Immediate2D.h"
#include "TY/Immediate3D.h"
#include "TY/Palette.h"
#include "Util/DebugTomlValue.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

namespace
{
    int findTargetWaypoint(
        const MachinePhysicsState& machineState,
        const SpatialData& spatialData,
        const SpatialWaypoint& currentWaypoint,
        Float3& targetDirection)
    {
        // レイキャスト風に前方を探索する
        std::optional<Float3> inwardCorrectionDir{};
        int targetWaypointIndex{};
        int lookaheadCount = 1;
        constexpr int maxLookaheadCount = 20;
        for (; lookaheadCount < maxLookaheadCount; ++lookaheadCount)
        {
            targetWaypointIndex =
                Modulo<int>(currentWaypoint.indexInList + lookaheadCount, spatialData.waypoints.size());

            auto& checkingWaypoint = spatialData.waypoints[targetWaypointIndex];

            if (checkingWaypoint.targetStrip.style != CourseSegmentStyle::Road)
            {
                break;
            }

            constexpr float margin = 5.0f;

            const Float3 leftBoundary = checkingWaypoint.targetStrip.leftmost + checkingWaypoint.right * margin;
            const Float3 rightBoundary = checkingWaypoint.targetStrip.rightmost - checkingWaypoint.right * margin;

            const Float3& currentPosition = machineState.m_pose.position;
            const Float3 leftBoundaryDir = (leftBoundary - currentPosition).normalized();
            const Float3 rightBoundaryDir = (rightBoundary - currentPosition).normalized();

            const Float3& v = machineState.m_velocity;
            const float vl = v.cross(leftBoundaryDir).dot(checkingWaypoint.normal());
            const float vr = v.cross(rightBoundaryDir).dot(checkingWaypoint.normal());
            if (Math::Sign(vl) == Math::Sign(vr))
            {
                // 速度方向が道から外れている
                if (Abs(vl) < Abs(vr))
                {
                    inwardCorrectionDir = rightBoundaryDir;
                }
                else
                {
                    inwardCorrectionDir = leftBoundaryDir;
                }

                break;
            }
        } // end for

        // -----------------------------------------------
        // targetDirection

        if (lookaheadCount == maxLookaheadCount ||
            currentWaypoint.targetStrip.style != CourseSegmentStyle::Road)
        {
            // 現在位置のパス方向に向くようにする
            targetDirection = currentWaypoint.forward;
        }
        else if (inwardCorrectionDir.has_value())
        {
            // 道の内側へ向かうようにする
            targetDirection = *inwardCorrectionDir;
        }
        else
        {
            // 現在の速度方向へ向くようにする
            targetDirection = machineState.m_velocity.normalized();
        }

#if defined(_DEBUG) && 0
        ImmediatePrint("lookaheadCount: {}", lookaheadCount);
#endif

        return targetWaypointIndex;
    }
}

namespace Race
{
    MachinePhysicsProps::input_t UpdateCharacterAILogic(CharacterAILogicState& state, const MachinePhysicsUnit& machine)
    {
        MachinePhysicsProps::input_t input{};

        const auto& machineState = machine.state;
        const auto& machineProps = machine.props;

        const auto& spatialData = GetRaceContext().spatialAI().data();
        const auto& currentLap = machineState.m_lapProgress;
        const auto& currentWaypoint = spatialData.fetchWaypoint(currentLap.segmentIndex, currentLap.stripIndex);

        const Float3& currentPosition = machineState.m_pose.position;
        const Float3& upVector = machineState.m_upVector;

        Float3 V = machineState.m_velocity;
        V = V - upVector * upVector.dot(V);
        V = V.normalized();

        Float3 targetDirection;
        const int targetWaypointIndex = findTargetWaypoint(machineState, spatialData, currentWaypoint, targetDirection);
        const SpatialWaypoint& targetWaypoint = spatialData.waypoints[targetWaypointIndex];

        Float3 wayNormal = targetWaypoint.normal();
        if (targetWaypoint.targetStrip.style == CourseSegmentStyle::Pipe)
        {
            Float3 n = (currentPosition - targetWaypoint.targetStrip.center);
            n = n - targetWaypoint.forward * targetWaypoint.forward.dot(n);
            wayNormal = -n.normalized();
        }
        else if (targetWaypoint.targetStrip.style == CourseSegmentStyle::Cylinder)
        {
            Float3 n = (currentPosition - targetWaypoint.targetStrip.center);
            n = n - targetWaypoint.forward * targetWaypoint.forward.dot(n);
            wayNormal = n.normalized();
        }

        // accelPressed
        input.accelPressed = true;

#if defined(_DEBUG) && 0
        ImmediatePrint("curveHeuristic: {:.02f}", targetWaypoint.curveHeuristic);
#endif

        // rightHandling, driftTrigger 
        float turningDemand;
        {
            Float3 F = machineState.m_forwardVector;
            F = F - wayNormal * wayNormal.dot(F);
            F = F.normalized();

            Float3 V = machineState.m_velocity;
            V = V - wayNormal * wayNormal.dot(V);
            V = V.normalized();

            const float dotF = targetDirection.dot(F);
            const float dotV = targetDirection.dot(V);
            const bool useF = dotF < dotV;
            turningDemand = 1.0f - Max(0.0f, useF ? dotF : dotV);

#if defined(_DEBUG)
            ImmediatePrint("turningIntensity: {:.02f}", turningDemand);
#endif

            if (turningDemand > 0.01f)
            {
                const Float3 cross = targetDirection.cross(useF ? F : V);
                const float rightSign = cross.dot(wayNormal) < 0.0f ? 1.0f : -1.0f;
                input.rightHandling = rightSign; // * Max(turningIntensity, 0.5f);
                input.driftTrigger = rightSign * (turningDemand > 0.1 ? 1.0f : 0.0f);

                if (turningDemand > 0.75f)
                {
                    input.hyperTurnRequested = true;
                }
            }
        }

        // pitch
        if (machineState.isHovering())
        {
            input.pitch = turningDemand < 0.5f ? -1.0f : 1.0f;
        }

        // cheatBoostFactor
        const float targeCheatBoost = state.m_inputCommand.targeCheatBoost;
        if (turningDemand > 0.75f && // 急カーブ
            not machineState.isHovering() && // 接地中
            targeCheatBoost > 1.0f)
        {
            // カーブ用のチート減速
            input.cheatBoostFactor = 1.0f;
        }
        else
        {
            input.cheatBoostFactor = targeCheatBoost;
        }

#if defined(_DEBUG)
        if (state.m_aiId == 0 &&
            GetDebugTomlValue<bool>("print_diagnostics"))
        {
            ImmediatePrint_TopRight("[CharacterAI#{}]", state.m_aiId);
            ImmediatePrint_TopRight("targetWaypoint: {}", targetWaypoint.indexInList);
            ImmediatePrint_TopRight("turningDemand: {:+.02f}", turningDemand);
            ImmediatePrint_TopRight("accelPressed: {}", input.accelPressed);
            ImmediatePrint_TopRight("rightHandling: {:+.02f}", input.rightHandling);
            ImmediatePrint_TopRight("driftTrigger: {:+.02f}", input.driftTrigger);
            ImmediatePrint_TopRight("velocity: {:.01f} km/h", machineState.m_velocity.length() * VelocityDisplayFactor);

            {
                const Float3 n = machineState.m_upVector;
                // Immediate3D::Line{machineState.m_pose.position + n, targetWaypoint.boundaryCenter + n}
                //     .setColor(Palette::Chartreuse)
                //     .pushAuto();
                Immediate3D::Line{
                        machineState.m_pose.position + n,
                        machineState.m_pose.position + n + targetDirection * 50.0f
                    }.setColor(Palette::Chartreuse)
                     .pushAuto();
                Immediate3D::Line{
                        machineState.m_pose.position + n,
                        machineState.m_pose.position + n + V * 50.0f
                    }.setColor(Palette::Coral)
                     .pushAuto();
                Immediate3D::Line{targetWaypoint.targetStrip.leftmost + n, targetWaypoint.targetStrip.rightmost + n}
                    .setColor(Palette::Aquamarine)
                    .pushAuto();
            }
        }
#endif

        return input;
    }
}
