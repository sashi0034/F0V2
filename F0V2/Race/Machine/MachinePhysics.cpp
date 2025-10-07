#include "pch.h"
#include "MachinePhysics.h"

#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"
#include "TY/GameStep.h"
#include "TY/GameTime.h"
#include "TY/Intersects3D.h"
#include "TY/Immediate3D.h"

using namespace Race;

namespace
{
    constexpr float epsGround = 1e-2f;

    struct HitTri
    {
        float moveDistance;
        Triangle3D tri;
        Plane3D plane;
        // Float3 intersection;
        // Float3 foot;
    };

    struct MoveResult
    {
        Float3 newPos;
        std::optional<HitTri> tri{};
    };

    MoveResult tryMoveCapsulePosition(const MachinePhysicsState& state, const Float3& fromPos, const Float3& toPos)
    {
        if (fromPos == toPos)
        {
            return {toPos, std::nullopt};
        }

        const auto moveTestCapsule = Capsule{fromPos, toPos, state.m_radius};

        const auto hit = GetRaceContext().stageManager().staticBvh().sphereCast(moveTestCapsule);
        if (not hit.has_value())
        {
            // 衝突なし
            return {toPos, std::nullopt};
        }

        // -----------------------------------------------
        // 衝突あり

        //             V  /|
        //               / |
        //              /  |
        //             /   |
        //          U /    |
        //           /+    |
        //          / +    |
        //         /  +    |
        //        /   +    |
        //       /    +    |
        //    T /--r--+    |    <-- normal
        //     /|     +    |
        //    / |     +    |
        //   /  |     +    |
        //  / θ |     +    |
        // /----+-----+----+
        // S    G     H    I

        const auto plane = hit->asPlane();

        const Float3 S = fromPos;
        const Float3 V = toPos;

        // const Float3 H = S - plane.normal * distance;

        // signedDistance: normal 方向が正
        const float signedDistanceHS = plane.signedDistanceFrom(S);
        const float signedDistanceIH = plane.signedDistanceFrom(V);

        const Float3 SV = V - S;
        const Float3 IV = SV - plane.normal * SV.dot(plane.normal);

        const float r = state.m_radius + epsGround;
        const float signedDistanceGS = signedDistanceHS - r;

        if (signedDistanceHS * signedDistanceIH >= 0)
        {
            // 移動ベクトルが面を貫通していない場合
            const Float3 G = S - plane.normal * signedDistanceGS;
            const Float3 T = G + IV;

            HitTri hitTri{};
            hitTri.moveDistance = (T - S).length();
            hitTri.tri = *hit;
            hitTri.plane = plane;
            return {T, hitTri};
        }
        else
        {
            // ST : SV = SG : SI
            const float lengthSV = SV.length();
            const float lengthSG = Abs(signedDistanceGS);
            const float lengthSI = Abs(-signedDistanceHS + signedDistanceIH);
            const float lengthST = (lengthSV * lengthSG / lengthSI);

            const Float3 ST = SV.normalized() * lengthST;
            const Float3 T = S + ST;

            HitTri hitTri{};
            hitTri.moveDistance = lengthST;
            hitTri.tri = *hit;
            hitTri.plane = plane;
            return {T, hitTri};
        }
    }

    void updateCapsulePosition(
        MachinePhysicsState& state,
        const MachinePhysicsProps& props,
        const Float3& fromPos, const Float3& moveVector, int nest = 0)
    {
        if (moveVector.lengthSq() < 1e-6f)
        {
            return;
        }

        const auto toPos = fromPos + moveVector;
        const auto [newPos, hitTris] = tryMoveCapsulePosition(state, fromPos, toPos);

        state.m_pose.position = newPos;

        if ((toPos - newPos).lengthSq() < 1e-6f)
        {
            return;
        }

        if (hitTris.has_value())
        {
            const auto& tri = *hitTris;
            const auto triCenter = tri.tri.centroid();

            if (props.debug.drawHitTris)
            {
                Immediate3D::Line{
                        triCenter,
                        triCenter + tri.plane.normal * 10
                    }.setColor(ColorF32{1.0f, 0.0f, 1.0f}, ColorF32{0.5f, 0, 0.5f})
                     .pushAuto();
                Immediate3D::LineSet{}
                    .appendTriangle(tri.tri.movedBy(tri.plane.normal * 0.1f))
                    .setColor(ColorF32{1.0f, 1.0f, 0.5f})
                    .pushAuto();
            }

            // 面の法線を採用
            const Float3 n = tri.plane.normal;

            // 法線方向速度の除去
            state.m_velocity = state.m_velocity - n * state.m_velocity.dot(n);

            // 法線方向移動ベクトルの補正
            const Float3 r = toPos - state.m_pose.position;
            Float3 newMoveVector = r - n * r.dot(n);
            if (newMoveVector.isZero())
            {
                return;
            }

            // 移動ベクトルの長さを残りの移動量に調節する
            newMoveVector = newMoveVector.normalized() * Max(0.0f, moveVector.length() - hitTris->moveDistance);

            if (nest < 3)
            {
                updateCapsulePosition(state, props, state.m_pose.position, newMoveVector, nest + 1);
            }
        }
    }

    // -----------------------------------------------

    void updateGroundedness(MachinePhysicsState& state)
    {
        // Float3 vector = state.m_velocity.normalized() * (state.m_radius + 0.1f);
        // if (vector.isZero())
        Float3 vector = -state.m_upVector * (state.m_radius + epsGround);

        const auto testCapsule = Capsule{state.m_pose.position, state.m_pose.position + vector, state.m_radius};
        const auto hit = GetRaceContext().stageManager().staticBvh().sphereCast(testCapsule);
        if (hit.has_value())
        {
            state.m_surfaceNormal = hit->getNormal();
            state.m_groundedness = state.m_surfaceNormal.dot(state.m_upVector);

            const auto plane = hit->asPlane();

            const Float3 S = state.m_pose.position;

            const float signedDistanceHS = plane.signedDistanceFrom(S);

            const float r = state.m_radius + epsGround;
            if (Abs(signedDistanceHS) < r)
            {
                // めり込んだ場合の位置の調整
                const float signedDistanceGS = signedDistanceHS - r;
                state.m_pose.position = S - plane.normal * signedDistanceGS;
            }
        }
        else
        {
            state.m_surfaceNormal = {};
            state.m_groundedness = 0.0f;
        }

        // std::cout << "groundedness: " << state.m_groundedness << std::endl;
    }

    // -----------------------------------------------

    int findNearestSegmentIndex(const Array<CourseSegment>& courseSegments, const Float3& position)
    {
        std::pair<int, float> bestSegment{-1, FLT_MAX};
        for (int i = 0; i < courseSegments.size(); ++i)
        {
            const auto& seg = courseSegments[i];
            const float dist = DistanceSq(position, LineSegment3D{seg.p1, seg.p2});
            if (dist < bestSegment.second)
            {
                bestSegment = {i, dist};
            }
        }

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

    Float3 calculateGravity(const Float3& position)
    {
        const auto& courseSegments = GetRaceContext().stageManager().courseSegments();

        const int nearestSegmentId = findNearestSegmentIndex(courseSegments, position);
        const auto& nearestSegment = courseSegments[nearestSegmentId];

        const int nearestStripId = findNearestStripIndex(nearestSegment, position);
        const auto& nearestStrip = nearestSegment.midwayStrips[nearestStripId];

        if (nearestSegment.style == CourseSegmentStyle::Tunnel &&
            not nearestStrip.tunnel.ringVectors[0].isZero()) // FIXME
        {
            // トンネル内の重力
            const auto& ringVectors = nearestStrip.tunnel.ringVectors;
            std::array<Float3, TunnelSubdivision> ringPoints{}; // リング上の仮点
            for (int i = 0; i < TunnelSubdivision; ++i)
            {
                const Float3 dir = ringVectors[i] + ringVectors[(i + 1) % TunnelSubdivision];
                ringPoints[i] = nearestStrip.center + dir;
            }

            float minDistSq = FLT_MAX;
            int minIndex{};
            for (int i = 0; i < TunnelSubdivision; ++i)
            {
                const float distSq = DistanceSq(position, ringPoints[i]);
                if (distSq < minDistSq)
                {
                    minDistSq = distSq;
                    minIndex = i;
                }
            }

            return (ringPoints[minIndex] - nearestStrip.center).normalized();
        }

        return -nearestStrip.normal;
    }
}

namespace Race
{
    void UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props)
    {
        const Float3 forwardVector = state.m_pose.rotation.rotate(Float3{0, 0, 1});

        const float airness = 1.0f - state.m_groundedness;
        state.m_velocity += state.m_gravity * airness * 50.0f * InGameDeltaTime();

        if (props.hasAccelInput)
        {
            state.m_velocity += forwardVector * 50.0f * InGameDeltaTime();
        }

        const float maxSpeed = 100.0f;
        if (state.m_velocity.lengthSq() > Math::Square(maxSpeed))
        {
            state.m_velocity = state.m_velocity.normalized() * maxSpeed;
        }

        // 移動処理
        {
            Float3 moveVector = state.m_velocity * InGameDeltaTime();

            moveVector += -state.m_upVector * airness * 10.0f * InGameDeltaTime(); // 常に微小量の力で地面方向に押し付ける

            updateCapsulePosition(state, props, state.m_pose.position, moveVector);
        }

        // 現在位置における重力方向を計算
        {
            state.m_gravity = calculateGravity(state.m_pose.position);

#if 0
            state.m_gravity = Float3(0, -1, 0);
#endif

            if (props.debug.drawHitTris)
            {
                Immediate3D::Line{
                        state.m_pose.position,
                        state.m_pose.position - state.m_gravity * 10
                    }.setColor(ColorF32{1.0f, 0.0f, 0.5f}, ColorF32{0.5f, 0, 0.5f})
                     .pushAuto();
            }
        }

        state.m_upVector = -state.m_gravity;

        // for (const auto dt : StandardStep_60Hz())
        // {
        //     state.m_upVector =
        //         state.m_upVector.slerp(-state.m_gravity, 10.0f * dt);
        // }

        constexpr Float3 v010{0, 1, 0};
        Quaternion targetRotation;
        if (v010.dot(state.m_upVector) > -0.999f)
        {
            targetRotation = Quaternion(v010, state.m_yaw);
            targetRotation *= Quaternion::FromUnitVectors(v010, state.m_upVector);
        }
        else
        {
            // 例外処理
            targetRotation = Quaternion(-v010, state.m_yaw);
        }

        // 滑らかに回転
        for (const auto dt : StandardStep_60Hz())
        {
            state.m_pose.rotation = state.m_pose.rotation.slerp(targetRotation, 10.0f * dt);

            // 空中にいるとき、滑らかに重力方向に向いていくようにする
            // TODO: 空中判定
            // state.m_actualSurfaceNormal = state.m_actualSurfaceNormal.slerp(-gravity, 1.0f * InGameDeltaTime());
        }

        updateGroundedness(state);

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
