#include "pch.h"
#include "MachinePhysics.h"

#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"
#include "TY/GameStep.h"
#include "TY/GameTime.h"
#include "TY/Intersects3D.h"
#include "TY/Shape3D.h"

using namespace Race;

namespace
{
    struct HitTri
    {
        float moveDistance;
        Triangle3D tri;
        Plane3D plane;
        Float3 intersection;
        Float3 foot;
    };

    struct MoveResult
    {
        Float3 newPos;
        std::optional<HitTri> tri{};
    };

    int s_triTestCount{};

    void tryMoveCapsulePosition_internal(
        const MachinePhysicsState& state,
        const Triangle3D& testTri,
        const Capsule& moveTestCapsule,
        const Float3& fromPos,
        const Float3& toPos,
        HitTri& hitTri,
        Float3& newPos)
    {
        s_triTestCount++;
        if (Intersects(moveTestCapsule, testTri))
        {
            //            U
            //           /|
            //          / |
            //         /  |
            //        /   |
            //       /    |
            //    T /--r--|
            //     /|     |
            //    / |     |
            //   /  |     |
            //  /   |     |
            // /----------|
            // S          H

            const auto lineST = Line3D::FromPoints(fromPos, toPos);

            auto plane = testTri.asPlane();
            if (Abs(lineST.normalizedDir.dot(plane.normal)) < 0.1f)
            {
                // 移動ベクトルと三角形がほぼ並行の場合
                // TODO: 対策考える
                return;
            }

            const Float3 S = fromPos;
            float lengthSU{};
            const auto tryU = IntersectsAt(lineST, plane, &lengthSU);
            if (not tryU)
            {
                return;
            }

            const Float3 U = *tryU;
            const Float3 SU = U - S;

            const float distance = plane.signedDistanceFrom(fromPos);
            const Float3 H = S - plane.normal * distance;
            const float lengthSH = Abs(distance);

            const Float3 SH = (H - S);

            const float r = state.m_radius + 1e-2f;

            const float SUoSH = SU.dot(SH);
            const float lengthST = lengthSU - r * (lengthSU * lengthSH) / Max(1e-30f, SUoSH);

            if (hitTri.moveDistance < lengthST)
            {
                return;
            }

            hitTri.moveDistance = lengthST;
            hitTri.tri = testTri;
            hitTri.plane = plane;
            hitTri.intersection = U;
            hitTri.foot = H;
            newPos = S + lineST.normalizedDir * lengthST;
        }
    }

    MoveResult tryMoveCapsulePosition(const MachinePhysicsState& state, const Float3& fromPos, const Float3& toPos)
    {
        if (fromPos == toPos)
        {
            return {toPos, {}};
        }

        const auto moveTestCapsule = Capsule{fromPos, toPos, state.m_radius};

        HitTri hitTri{};
        hitTri.moveDistance = FLT_MAX;
        Float3 newPos = toPos;

        const auto hits = GetRaceContext().stageManager().staticBvh().queryHits(moveTestCapsule.aabb());
        hits.forEachTriangle([&](const Triangle3D& tri)
        {
            tryMoveCapsulePosition_internal(state, tri, moveTestCapsule, fromPos, toPos, hitTri, newPos);
        });

        return {newPos, hitTri};
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
                Shape3D::Line{
                        triCenter,
                        triCenter + tri.plane.normal * 10
                    }.setColor(ColorF32{1.0f, 0.0f, 1.0f}, ColorF32{0.5f, 0, 0.5f})
                     .pushAuto();
                Shape3D::LineSet{}
                    .appendTriangle(tri.tri.movedBy(tri.plane.normal * 0.1f))
                    .setColor(ColorF32{1.0f, 1.0f, 0.5f})
                    .pushAuto();
            }

            // 面の法線を採用
            state.m_surfaceNormal = tri.plane.normal;

            const Float3 n = state.m_surfaceNormal;

            // 法線方向速度の除去
            state.m_velocity = state.m_velocity - n * state.m_velocity.dot(n);

            // 法線方向移動ベクトルの除去
            const Float3 r = toPos - state.m_pose.position;
            const auto newMoveVector = r - n * r.dot(n);

            if (nest < 3)
            {
                updateCapsulePosition(state, props, state.m_pose.position, newMoveVector, nest + 1);
            }
        }
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
}

namespace Race
{
    void UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props)
    {
        // 現在位置における重力方向を計算
        {
            const auto& courseSegments = GetRaceContext().stageManager().courseSegments();

            const int nearestSegmentId = findNearestSegmentIndex(courseSegments, state.m_pose.position);
            const auto& nearestSegment = courseSegments[nearestSegmentId];

            const int nearestStripId = findNearestStripIndex(nearestSegment, state.m_pose.position);
            const auto& nearestStrip = nearestSegment.midwayStrips[nearestStripId];

            const Float3 n0 = -nearestStrip.normal;

            state.m_gravity = n0;

            if (props.debug.drawHitTris)
            {
                Shape3D::Line{
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

        const Float3 forwardVector = state.m_pose.rotation.rotate(Float3{0, 0, 1});

        state.m_velocity += state.m_gravity * 50.0f * InGameDeltaTime();

        if (props.hasAccelInput)
        {
            state.m_velocity += forwardVector * 50.0f * InGameDeltaTime();
        }

        const float maxSpeed = 100.0f;
        if (state.m_velocity.lengthSq() > Math::Square(maxSpeed))
        {
            state.m_velocity = state.m_velocity.normalized() * maxSpeed;
        }

        Float3 moveVector = state.m_velocity * InGameDeltaTime();

        moveVector += -state.m_upVector * 10.0f * InGameDeltaTime(); // 常に微小量の力で地面方向に押し付ける

        updateCapsulePosition(state, props, state.m_pose.position, moveVector);

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
