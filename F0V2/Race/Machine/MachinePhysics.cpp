#include "pch.h"
#include "MachinePhysics.h"

#include "MachineMoveResolver.h"
#include "GM/DebugService.h"
#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"
#include "TY/GameStep.h"
#include "TY/GameTime.h"
#include "TY/Intersects3D.h"
#include "TY/Immediate3D.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

#define DEBUG_DRAW_LINES

namespace
{
    Float3 updateUpVector(const MachinePhysicsState& state)
    {
        Float3 upVector = state.m_upVector;
        if (state.isHovering())
        {
            // 空中にいるときは滑らかに重力方向へ m_upVector を調整
            for (const auto dt : StandardStep_60Hz())
            {
                upVector = state.m_upVector.slerp(-state.m_gravity, dt * 5.0f);
            }
        }
        else
        {
            // 地面接触時はその地面の m_surfaceNormal を m_upVector に設定
            assert(not state.m_surfaceNormal.isZero());
            upVector = state.m_surfaceNormal;
        }

        return upVector;
    }

    // -----------------------------------------------

    int findNearestSegmentIndex(const Array<CourseSegment>& courseSegments, const Float3& position)
    {
        std::pair<int, float> bestSegment{-1, FLT_MAX};
        for (int i = 0; i < courseSegments.size(); ++i)
        {
            const auto& seg = courseSegments[i];
            if (seg.style == CourseSegmentStyle::Gap)
            {
                continue;;
            }

            const float dist = DistanceSq(position, LineSegment3D{seg.p1, seg.p2});
            if (dist < bestSegment.second)
            {
                bestSegment = {i, dist};
            }
        }

        assert(bestSegment.first != -1);
        return bestSegment.first;
    }

    // TODO: 最適化
    int findNearestStripIndex(const CourseSegment& segment, const Float3& position)
    {
        const auto& strips = segment.midwayStrips;
        std::pair<int, float> bestStrip{-1, FLT_MAX};
        for (int i = 0; i < strips.size(); ++i)
        {
            const auto& strip = strips[i];
            const float dist = DistanceSq(position, strip.center + strip.toNext * 0.5f);
            if (dist < bestStrip.second)
            {
                bestStrip = {i, dist};
            }
        }

        return bestStrip.first;
    }

    SegmentAndStrip findNearestSegmentAndStrip(const Array<CourseSegment>& courseSegments, const Float3& position)
    {
        const int segmentId = findNearestSegmentIndex(courseSegments, position);
        const auto& segment = courseSegments[segmentId];

        const int stripId = findNearestStripIndex(segment, position);

        return SegmentAndStrip{segmentId, stripId};
    }

    // TODO: 削除していいかも
    const CourseStrip& getNextStrip(const Array<CourseSegment>& courseSegments, int segmentId, int stripId)
    {
        const auto& segment = courseSegments[segmentId];
        if (stripId + 1 < segment.midwayStrips.size())
        {
            return segment.midwayStrips[stripId + 1];
        }
        else
        {
            const auto& nextSegment = courseSegments[(segmentId + 1) % courseSegments.size()];
            return nextSegment.midwayStrips[1]; // [0] は segment.midwayStrips[^1] と同じなので [1] を返す
        }
    }

    Float3 calculateGravity(const MachinePhysicsState& state, const SegmentAndStrip& targetSegmentAndStrip)
    {
        const Float3& position = state.m_pose.position;

        const auto& courseSegments = GetRaceContext().stageManager().courseSegments();

        const auto& targetSegment = courseSegments[targetSegmentAndStrip.segmentIndex];

        const auto& targetStrip = targetSegment.midwayStrips[targetSegmentAndStrip.stripIndex];

        if (targetStrip.style == CourseSegmentStyle::Pipe)
        {
            assert(not targetStrip.pipe.ringVectors[0].isZero());

            const auto line = Line3D::FromPoints(targetStrip.center, targetStrip.center + targetStrip.toNext);
            const Float3 p = line.projectPoint(position);
            return (position - p).normalized();
        }
        else if (targetStrip.style == CourseSegmentStyle::Cylinder)
        {
            assert(not targetStrip.pipe.ringVectors[0].isZero());

            const auto line = Line3D::FromPoints(targetStrip.center, targetStrip.center + targetStrip.toNext);
            const Float3 p = line.projectPoint(position);
            return (p - position).normalized();
        }

        return -targetStrip.normal;
    }

    void applyInputAccel(MachinePhysicsState& state, const MachinePhysicsProps& props)
    {
        assert(props.input.accelPressed);

        const float currentVelocity = state.m_velocity.length();
        const float targetVelocity = props.peakVelocity;

        float dv = targetVelocity - currentVelocity;
        dv = Max(dv, 0.1f);

        float rate = props.accelFactor;
        rate = Max(rate, Min(1.0f / rate, currentVelocity / (rate * targetVelocity)));

        state.m_velocity += state.m_forwardVector * dv * rate * InGameDeltaTime();
    }
}

namespace Race
{
    Float3 MachinePhysicsState::rightVector() const
    {
        return m_upVector.cross(m_forwardVector).normalized();
    }

    bool MachinePhysicsState::isHovering() const
    {
        return m_surfaceNormal.isZero();
    }

    void UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props)
    {
        // const Float3 gravity = state.m_gravity - state.m_surfaceNormal * state.m_surfaceNormal.dot(state.m_gravity);
        Float3 gravity = state.m_gravity; // TODO: 地面方向の成分を除去
        state.m_velocity += gravity * 50.0f * InGameDeltaTime();

        if (props.input.accelPressed)
        {
            applyInputAccel(state, props);
        }

        if (props.input.boostRequested && state.m_manualBoost < 0.5f)
        {
            state.m_manualBoost = 1.0f;
            state.m_durability = PositiveF32(state.m_durability - 800.0f);
        }

        constexpr float maxVelocity = 500.0f;
        if (state.m_velocity.lengthSq() > Math::Square(maxVelocity))
        {
            state.m_velocity = state.m_velocity.normalized() * maxVelocity;
        }

        // ブースト処理
        if (state.m_manualBoost > 0.0f)
        {
            state.m_velocity += state.m_forwardVector * 150.0f * Min(1.0f, state.m_manualBoost) * InGameDeltaTime();

            state.m_manualBoost = Max<float>(0.0f, state.m_manualBoost - InGameDeltaTime());
        }

        if (state.m_passiveBoost > 0.0f)
        {
            state.m_velocity += state.m_forwardVector * 100.0f * Min(1.0f, state.m_passiveBoost) * InGameDeltaTime();

            state.m_passiveBoost = Max<float>(0.0f, state.m_passiveBoost - InGameDeltaTime());
        }

        // ドリフト操作
        const float driftTrigger = props.input.driftTrigger; // state.isHovering() ? 0.0f : props.input.driftTrigger;
        if (driftTrigger != 0.0f)
        {
            state.m_driftOffset += static_cast<float>(driftTrigger) * InGameDeltaTime();
            constexpr float maxSlipOffset = 5.0f;
            if (Abs(state.m_driftOffset) > maxSlipOffset)
            {
                state.m_driftOffset = maxSlipOffset * Math::Sign(state.m_driftOffset);
            }
        }

        if (Math::Sign(driftTrigger) != Math::Sign(state.m_driftOffset))
        {
            // ドリフト量を減らす
            const float delta = 10.0f * InGameDeltaTime(); // > 0
            state.m_driftOffset -= delta * Math::Sign(state.m_driftOffset);
            if (Abs(state.m_driftOffset) < delta)
            {
                state.m_driftOffset = 0.0f;
            }
        }

        Float3 slippedForwardVector = state.m_forwardVector + state.rightVector() * state.m_driftOffset;
        slippedForwardVector = slippedForwardVector.normalized();

        // 移動処理
        {
            Float3 moveVector = state.m_velocity * InGameDeltaTime();

            ResolveMachineMove(state, moveVector, props);
        }

        const auto& courseSegments = GetRaceContext().stageManager().courseSegments();

        const auto nearestSegmentAndStrip = findNearestSegmentAndStrip(courseSegments, state.m_pose.position);

        // ラップ更新
        {
            state.m_lapProgress = EvaluateLapProgress(state.m_lapProgress, nearestSegmentAndStrip);

            if (state.m_reachedLapProgress.isLessThan(state.m_lapProgress))
            {
                state.m_reachedLapProgress = state.m_lapProgress;
            }
        }

        // 現在位置における重力方向を計算
        {
            state.m_gravity = calculateGravity(state, nearestSegmentAndStrip);

#if 0
            state.m_gravity = Float3(0, -1, 0);
#endif

#if defined(_DEBUG) && defined(DEBUG_DRAW_LINES)
            Immediate3D::Line{

                    state.m_pose.position,
                    state.m_pose.position - state.m_gravity * 10
                }.setColor(ColorF32{0.3f, 0.0f, 0.3f}, ColorF32{0.1f, 0, 0.1f})
                 .pushAuto();
#endif
        }

        // -----------------------------------------------

        for (const float dt : StandardStep_60Hz())
        {
            // 左ジョイスティック操作
            const float rightHandling = props.input.rightHandling;
            float rightShift;
            if (state.m_driftOffset != 0.0f)
            {
                float r = rightHandling * std::sqrtf(Abs(state.m_driftOffset)) * Math::Sign(state.m_driftOffset);
                constexpr float boundaryR = 0.5f;
                if (r < 0.0f)
                {
                    // r = boundaryR - boundaryR * (1.0f - 1.0f / (1.0f + r * r));
                    r = boundaryR / (1.0f - r);
                }
                else
                {
                    constexpr float driftFactor = 0.5f; // TODO: マシンごとのパラメータにする
                    r = boundaryR + driftFactor * r;
                }

                r *= Math::Sign(state.m_driftOffset);

                const float velocityLength = state.m_velocity.length();
                state.m_velocity = state.m_velocity + state.rightVector() * r;
                state.m_velocity = state.m_velocity.normalized() * velocityLength;

                rightShift = r;
            }
            else
            {
                rightShift = rightHandling;
            }

            constexpr float steeringSensitivity = 0.015f;
            state.m_forwardVector += state.rightVector() * rightShift * steeringSensitivity;
            state.m_forwardVector = state.m_forwardVector.normalized();
        }

        // -----------------------------------------------
        // m_forwardVector

        state.m_forwardVector =
            state.m_forwardVector - state.m_upVector * state.m_upVector.dot(state.m_forwardVector);

        if (state.m_forwardVector.isZero())
        {
            state.m_forwardVector = Mat4x4{state.m_pose.rotation}.forward();
            state.m_forwardVector =
                state.m_forwardVector - state.m_upVector * state.m_upVector.dot(state.m_forwardVector);

            if (state.m_forwardVector.isZero())
            {
                assert(false); // TODO
            }
        }

        state.m_forwardVector = state.m_forwardVector.normalized();

        // -----------------------------------------------

#if defined(_DEBUG)
        if (props.machineId == g_debugService.monitorMachineId)
        {
            ImmediatePrint_TopCenter(
                "[{}] Lap: {}, Segment: {}, Strip: {}",
                props.machineId,
                state.m_lapProgress.lapIndex,
                state.m_lapProgress.segmentIndex,
                state.m_lapProgress.stripIndex);
            ImmediatePrint_MiddleCenter("position: {:.02f}", state.m_pose.position);
            ImmediatePrint_MiddleCenter("m_forwardVector: {:.02f}", state.m_forwardVector);
            ImmediatePrint_MiddleCenter("m_upVector: {:.02f}", state.m_upVector);
            ImmediatePrint_MiddleCenter("m_gravity: {:.02f}", state.m_gravity);
        }
#endif

        const Quaternion targetRotation =
            Quaternion::FromUnitVectors(Float3{0, 0, 1}, slippedForwardVector); // TODO: pitch

        // 滑らかに回転
        for (const auto dt : StandardStep_60Hz())
        {
            state.m_pose.rotation = state.m_pose.rotation.slerp(targetRotation, 10.0f * dt);
        }

        state.m_upVector = updateUpVector(state);

        ResolveMachineGroundContact(state);

        // 速度の減衰
        for (const auto dt : StandardStep_60Hz())
        {
            constexpr float mu = 0.5f; // TODO
            if (state.m_velocity.lengthSq() > Math::Square(mu))
            {
                state.m_velocity -= state.m_velocity.normalized() * mu;
            }
            else
            {
                state.m_velocity = {};
            }
        }
    }
}
