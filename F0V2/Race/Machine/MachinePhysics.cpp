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
}

namespace Race
{
    void UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props)
    {
        state.m_pose.rotation = Quaternion(
            Float3{0, 1, 0,}, state.m_yaw) * Quaternion::FromUnitVectors(Float3{0, 1, 0}, state.m_surfaceNormal);

        const Float3 forwardVector = state.m_pose.rotation.rotate(Float3{0, 0, 1});

        Float3 gravity = Float3{0.0f, -1.0f, 0.0f};
        state.m_velocity += gravity * InGameDeltaTime();

        if (props.hasAccelInput)
        {
            state.m_velocity += forwardVector * 1.5f * InGameDeltaTime();
        }

        Float3 moveVector = state.m_velocity;

        updateCapsulePosition(state, props, state.m_pose.position, moveVector);

        constexpr float mu = 0.5f; // TODO
        if (state.m_velocity.lengthSq() > Math::Square(mu * InGameDeltaTime()))
        {
            state.m_velocity -= state.m_velocity.normalized() * mu * InGameDeltaTime();
        }
        else
        {
            state.m_velocity = {};
        }
    }
}
