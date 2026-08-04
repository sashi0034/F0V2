#include "pch.h"
#include "MachinePhysics.h"

#include "MachineMoveResolver.h"
#include "GM/DebugService.h"
#include "Race/IRaceContext.h"
#include "Race/Common/RaceSharedState.h"
#include "Race/Stage/StageManager.h"
#include "TY/GameStep.h"
#include "TY/GameTime.h"
#include "TY/Intersects3D.h"
#include "TY/Immediate3D.h"
#include "Util/DebugTomlValue.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

namespace
{
    Float3 updateUpVector(const MachinePhysicsState& state)
    {
        if (state.isHovering())
        {
            // 空中にいるときは滑らかに重力方向へ m_upVector を調整
            return -state.m_gravity;
        }
        else
        {
            // 地面接触時はその地面の m_surfaceNormal を m_upVector に設定
            assert(not state.m_surfaceNormal.isZero());
            return state.m_surfaceNormal;
        }
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
                continue;
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
    int findNearestStripIndex(const CourseSegment& segment, const Float3& position, float* outDistanceSq)
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

        if (outDistanceSq)
        {
            *outDistanceSq = bestStrip.second;
        }

        return bestStrip.first;
    }

    struct NearestSegmentAndStrip
    {
        int segmentIndex;
        int stripIndex;
        float distanceSq;

        operator SegmentAndStrip() const
        {
            return SegmentAndStrip{segmentIndex, stripIndex};
        }
    };

    NearestSegmentAndStrip findNearestSegmentAndStrip(
        const Array<CourseSegment>& courseSegments, const Float3& position)
    {
        const int segmentId = findNearestSegmentIndex(courseSegments, position);
        const auto& segment = courseSegments[segmentId];

        float distanceSq{};
        const int stripId = findNearestStripIndex(segment, position, &distanceSq);

        return NearestSegmentAndStrip{segmentId, stripId, distanceSq};
    }

    constexpr float gravityDistanceThreshold = 50.0f;

    Float3 calculateGravity(const MachinePhysicsState& state, const NearestSegmentAndStrip& nearestSegmentAndStrip)
    {
        const Float3& position = state.m_pose.position;

        if (nearestSegmentAndStrip.distanceSq >= Math::Square(gravityDistanceThreshold))
        {
            // コースから遠すぎる場合は単純に鉛直下向きへ重力をかける
            return Float3{0.0f, -1.0f, 0.0f};
        }

        const auto& courseSegments = GetRaceContext().stageManager().courseSegments();

        const auto& nearestSegment = courseSegments[nearestSegmentAndStrip.segmentIndex];

        const auto& nearestStrip = nearestSegment.midwayStrips[nearestSegmentAndStrip.stripIndex];

        if (nearestStrip.style == CourseSegmentStyle::Pipe)
        {
            assert(not nearestStrip.pipe.ringVectors[0].isZero());

            const auto line = Line3D::FromPoints(nearestStrip.center, nearestStrip.center + nearestStrip.toNext);
            const Float3 p = line.projectPoint(position);
            return (position - p).normalized();
        }
        else if (nearestStrip.style == CourseSegmentStyle::Cylinder)
        {
            assert(not nearestStrip.pipe.ringVectors[0].isZero());

            const auto line = Line3D::FromPoints(nearestStrip.center, nearestStrip.center + nearestStrip.toNext);
            const Float3 p = line.projectPoint(position);
            return (p - position).normalized();
        }

        return -nearestStrip.normal;
    }

    bool isFallingOffCourse(const MachinePhysicsState& state, const NearestSegmentAndStrip& nearestSegmentAndStrip)
    {
        const auto& courseSegments = GetRaceContext().stageManager().courseSegments();

        const auto& nearestSegment = courseSegments[nearestSegmentAndStrip.segmentIndex];

        const auto& nearestStrip = nearestSegment.midwayStrips[nearestSegmentAndStrip.stripIndex];

        // nearestStrip.y と比べてかなり下まで落ちているなら落下と判定
        return nearestStrip.center.y - state.m_pose.position.y > gravityDistanceThreshold;
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

        const float cheat = props.input.cheatBoostFactor;
        state.m_velocity += state.m_forwardVector * dv * rate * cheat * InGameDeltaTime();
    }

    bool isBoostUnlocked(const MachinePhysicsState& state)
    {
#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("boost_unlocked"))
        {
            return true;
        }
#endif
        // 二周目からブースト使用可能
        return state.m_lapProgress.lapIndex >= 1;
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

    bool MachinePhysicsState::isHitDetectionEnabled() const
    {
        return not m_isRunningEventProcess;
    }

    bool MachinePhysicsState::isBoostUnlocked() const
    {
        return ::isBoostUnlocked(*this);
    }

    bool MachinePhysicsState::isDead() const
    {
        return m_durability <= 0.0f;
    }

    void SetupMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props)
    {
        const auto startPosition = GetRaceContext().stageManager().getStartPosition(props.machineId);

        state = {};

        state.m_pose.position = startPosition.position;

        const Float3 startRight = startPosition.up.cross(startPosition.forward).normalized();
        const Float3 startUp = startPosition.forward.cross(startRight).normalized();
        state.m_pose.rotation = Quaternion::FromAxes(startRight, startUp, startPosition.forward);

        state.m_forwardVector = startPosition.forward;

        state.m_upVector = startPosition.up;

        state.m_durability = props.maxDurability;
    }

    MachinePhysicsUpdateOutcome UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props)
    {
        MachinePhysicsUpdateOutcome updateOutcome{};

        if (not g_sharedState->isRaceStarted || state.m_isRunningEventProcess)
        {
            return updateOutcome;
        }

        // const Float3 gravity = state.m_gravity - state.m_surfaceNormal * state.m_surfaceNormal.dot(state.m_gravity);
        Float3 gravity = state.m_gravity; // FIXME: 地面方向の成分を除去?

        float gravityFactor = 50.0f;

        // ピッチに応じた重力方向移動量
        if (state.isHovering())
        {
            if (state.m_pitchRate < 0.0f)
            {
                gravityFactor *= 1.0f + (-state.m_pitchRate) * 4.0f;
            }
            else if (state.m_pitchRate > 0.0f)
            {
                gravityFactor *= 1.0f - state.m_pitchRate * 0.5f;
            }
        }

        state.m_velocity += gravity * gravityFactor * InGameDeltaTime();

        auto deviceInput = props.input;
        if (state.isDead())
        {
            deviceInput = {};
        }

        if (deviceInput.accelPressed)
        {
            applyInputAccel(state, props);

            updateOutcome.accelInputAccepted = true;
        }

        // ブースト入力処理
        state.m_manualBoostCooldownTime = Max(0.0f, state.m_manualBoostCooldownTime - InGameDeltaTime());

        state.m_boostComboCountdown = Max(0.0f, state.m_boostComboCountdown - InGameDeltaTime());
        if (state.m_boostComboCountdown <= 0.0f)
        {
            state.m_boostComboCount = 0;
        }

        if (deviceInput.boostRequested)
        {
            if (isBoostUnlocked(state) &&
                state.m_durability > props.boostCost &&
                state.m_manualBoost < 0.1f &&
                state.m_manualBoostCooldownTime <= 0.0f)
            {
                if (state.m_boostComboCountdown > 0.0f)
                {
                    // コンボ発生
                    state.m_boostComboCount++;
                }

                const float comboBonus = state.m_boostComboCount * 0.1f; // TODO: 調整
                state.m_manualBoost = 1.0f + comboBonus;
                state.m_manualBoostCooldownTime = 2.0f + comboBonus;

                const float comboGracePeriod = state.m_boostComboCount == 0 ? 1.0f : 0.5f;
                state.m_boostComboCountdown = state.m_manualBoostCooldownTime + comboGracePeriod;

                state.m_durability = PositiveF32(state.m_durability - props.boostCost);

                updateOutcome.boostInputAccepted = true;
            }
            else
            {
                // コンボ中止
                state.m_boostComboCountdown = 0.0f;
                state.m_boostComboCount = 0;
            }
        }

        // 最大速度制限
        constexpr float maxVelocity = 500.0f;
        if (state.m_velocity.lengthSq() > Math::Square(maxVelocity))
        {
            state.m_velocity = state.m_velocity.normalized() * maxVelocity;
        }

        // ブースト処理
        if (state.m_manualBoost > 0.0f)
        {
            // const float comboBonus = 1.0f + state.m_boostComboCount * 0.1f;
            const float speed = 150.0f * Min(1.0f, state.m_manualBoost);
            state.m_velocity += state.m_forwardVector * speed * InGameDeltaTime();

            state.m_manualBoost = Max<float>(0.0f, state.m_manualBoost - InGameDeltaTime());
        }

        if (state.m_passiveBoost > 0.0f)
        {
            state.m_velocity += state.m_forwardVector * 100.0f * Min(1.0f, state.m_passiveBoost) * InGameDeltaTime();

            state.m_passiveBoost = Max<float>(0.0f, state.m_passiveBoost - InGameDeltaTime());
        }

        // ハイパーターン
        const bool canTriggerHyperTurn = state.m_hyperTurnTime == 0.0f; // && not state.m_stabilizingAfterHyperTurn;
        if (canTriggerHyperTurn && deviceInput.hyperTurnRequested)
        {
            state.m_hyperTurn = 0.0f;
            state.m_hyperTurnTime = 0.1f;
            state.m_stabilizingAfterHyperTurn = false;

            state.m_velocity = state.m_velocity * 0.95f;
        }

        if (state.m_hyperTurnTime > 0.0f)
        {
            // 傾く
            const float impulseIntensity = 0.5 + Min(state.m_velocity.length(), 100.0f) / 100.0f;
            state.m_hyperTurn += deviceInput.rightHandling * impulseIntensity * InGameDeltaTime();

            state.m_hyperTurnTime = Max<float>(0.0f, state.m_hyperTurnTime - InGameDeltaTime());
            if (state.m_hyperTurnTime == 0.0f)
            {
                state.m_stabilizingAfterHyperTurn = true;
            }
        }

        if (state.m_stabilizingAfterHyperTurn)
        {
            // 体制復帰
            const auto s = Math::Sign(state.m_hyperTurn);
            state.m_hyperTurn -= s * 5.0f * InGameDeltaTime();
            if (s != Math::Sign(state.m_hyperTurn))
            {
                state.m_hyperTurn = 0.0f;
                state.m_stabilizingAfterHyperTurn = false;
            }
        }

        // ピッチ操作
        if (state.isHovering())
        {
            if (Abs(deviceInput.pitch) < 0.5f || Math::Sign(deviceInput.pitch) != Math::Sign(state.m_rawPitchRate))
            {
                // 弱い入力なら減衰
                for (const auto dt : StandardStep_60Hz())
                {
                    state.m_rawPitchRate *= 0.5f;
                }
            }

            constexpr float speed = 2.0f;
            state.m_rawPitchRate += speed * deviceInput.pitch * InGameDeltaTime();
            if (Abs(state.m_rawPitchRate) > 1.0f)
            {
                state.m_rawPitchRate = 1.0f * Math::Sign(state.m_rawPitchRate);
            }
        }
        else
        {
            state.m_rawPitchRate = 0.0f;
        }

        state.m_pitchRate = std::sqrt(Abs(state.m_rawPitchRate)) * Math::Sign(state.m_rawPitchRate);
        // ImmediatePrint_MiddleCenter("{}", state.m_rawPitchRate);

        // ドリフト操作
        const float driftTrigger = deviceInput.driftTrigger; // state.isHovering() ? 0.0f : deviceInput.driftTrigger;
        if (driftTrigger != 0.0f)
        {
            constexpr float maxSlipOffset = 1.0f;
            const float targetDriftOffset = maxSlipOffset * (driftTrigger * 1.5f + deviceInput.rightHandling * 1.45f);

            const float d = InGameDeltaTime() * 10.0f;
            if (Abs(targetDriftOffset - state.m_driftOffset) < d)
            {
                state.m_driftOffset = targetDriftOffset;
            }
            else
            {
                state.m_driftOffset += Math::Sign(targetDriftOffset - state.m_driftOffset) * d;
            }

            updateOutcome.driftInputAccepted = true;
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
        state.m_visualForwardVector = slippedForwardVector + state.m_upVector * state.m_pitchRate;

        slippedForwardVector = slippedForwardVector.normalized();
        state.m_visualForwardVector = state.m_visualForwardVector.normalized();

        // 移動処理
        {
            Float3 moveVector = state.m_velocity * InGameDeltaTime();

            ResolveMachineMove(state, moveVector, props);
        }

        const auto& courseSegments = GetRaceContext().stageManager().courseSegments();

        const auto nearestSegmentAndStrip = findNearestSegmentAndStrip(courseSegments, state.m_pose.position);

        // ラップ更新
        // if (not state.isHovering())
        {
            state.m_lapProgress = EvaluateLapProgress(state.m_lapProgress, nearestSegmentAndStrip);

            if (not state.isHovering() &&
                state.m_markedLapProgress.isLessThan(state.m_lapProgress))
            {
                state.m_markedLapProgress = state.m_lapProgress;
            }
        }

        // 現在位置における重力方向を計算
        {
            state.m_gravity = calculateGravity(state, nearestSegmentAndStrip);

#if 0
            state.m_gravity = Float3(0, -1, 0);
#endif

#if defined(_DEBUG)
            if (GetDebugTomlValue<bool>("draw_physics_lines"))
            {
                Immediate3D::Line{
                        state.m_pose.position,
                        state.m_pose.position - state.m_gravity * 10
                    }.setColor(ColorF32{0.3f, 0.0f, 0.3f}, ColorF32{0.1f, 0, 0.1f})
                     .pushAuto();
            }
#endif
        }

        // コース外で落下中か判定
        state.m_isFallingOffCourse = isFallingOffCourse(state, nearestSegmentAndStrip);

        // -----------------------------------------------

        for (const float dt : StandardStep_60Hz())
        {
            // 左ジョイスティック操作: 左右
            const float rightHandling = deviceInput.rightHandling;
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

                rightShift = r;
            }
            else
            {
                rightShift = rightHandling;
            }

            constexpr float steeringSensitivity = 0.015f;
            state.m_forwardVector += state.rightVector() * (rightShift * steeringSensitivity);
            state.m_forwardVector = state.m_forwardVector.normalized();

            if (state.m_hyperTurn != 0.0f)
            {
                // 速度偏向
                state.m_forwardVector += state.rightVector() * state.m_hyperTurn * 1.0f;
                state.m_forwardVector = state.m_forwardVector.normalized();

                // state.m_velocity =
                //     Quaternion::FromUnitVectors(previousForward, state.m_forwardVector).rotate(state.m_velocity);

                // const float t = Min(0.1f, Abs(state.m_hyperTurn));
                // state.m_velocity =
                //     state.m_velocity.normalized().slerp(state.m_forwardVector, t) * state.m_velocity.length();

                const Float3 upVector =
                    (state.m_upVector - state.m_forwardVector * state.m_forwardVector.dot(state.m_upVector))
                    .normalized();
                if (not upVector.isZero())
                {
                    const Float3 upVelocity = upVector * upVector.dot(state.m_velocity);
                    Float3 v = state.m_velocity - upVelocity;

                    const float t = Min(0.1f, Abs(state.m_hyperTurn));
                    v = v.length() * v.normalized().safe_slerp(state.m_forwardVector, t, state.m_upVector);

                    state.m_velocity = upVelocity + v;
                }
            }
        }

        // -----------------------------------------------
        // m_upVector, m_forwardVector

        state.m_upVector = updateUpVector(state);

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
        if (props.machineId == g_debugService.monitorMachineId &&
            GetDebugTomlValue<bool>("print_diagnostics"))
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

        // ビューのクォータニオン作成
        {
            const float rollAmount = deviceInput.rightHandling * 0.5f + Math::Sign(state.m_hyperTurn) * 0.5f;
            const Quaternion rollRotation{state.m_visualForwardVector, -rollAmount};

            const Float3 visualRightVector =
                rollRotation.rotate(state.m_upVector.cross(state.m_visualForwardVector).normalized());
            const Float3 visualUpVector =
                rollRotation.rotate(state.m_visualForwardVector.cross(visualRightVector).normalized());

            const Quaternion targetRotation =
                Quaternion::FromAxes(visualRightVector, visualUpVector, state.m_visualForwardVector);

            // 滑らかに回転
            for (const auto dt : StandardStep_60Hz())
            {
                state.m_pose.rotation = state.m_pose.rotation.slerp(targetRotation, 10.0f * dt);
            }
        }

        const Float3 slippedRightVector = state.m_upVector.cross(slippedForwardVector).normalized();
        const Float3 slippedUpVector = slippedForwardVector.cross(slippedRightVector).normalized();

        ResolveMachineGroundContact(state);

        // 速度の偏向 (向きを　slippedForwardVector に近づける)
        {
            const Float3 upVelocity = slippedUpVector * slippedUpVector.dot(state.m_velocity);

            Float3 v = state.m_velocity - upVelocity;
            const float forwardRightSq = v.lengthSq();

            const Float3& fv = slippedForwardVector;
            const Float3& rv = slippedRightVector;
            v = v - rv * rv.dot(v) * InGameDeltaTime() * 0.5f;

            const float newForwardLength = std::sqrt(forwardRightSq - Math::Square(rv.dot(v)));
            constexpr float attenuation = 0.85f;
            v = v + fv * (newForwardLength - fv.dot(v)) * attenuation;

            // ImmediatePrint_MiddleCenter("{:.02f}", v.length() - std::sqrt(previousSpeedSq));

            state.m_velocity = v + upVelocity;
        }

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

        return updateOutcome;
    }
}
