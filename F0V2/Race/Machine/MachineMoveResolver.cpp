#include "pch.h"
#include "MachineMoveResolver.h"

#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"
#include "Race/Stage/StageStaticCollider.h"
#include "TY/Color.h"
#include "TY/GameTime.h"
#include "TY/Immediate3D.h"
#include "TY/IndexedTriangle.h"
#include "TY/Intersects3D.h"
#include "Util/DebugTomlValue.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

namespace
{
    constexpr float EPS_CONTACT = 1e-2f;

    void debugDrawTriangle(const IndexedTriangle& tri, const Float3 normal)
    {
        const auto triCenter = tri.centroid();
        Immediate3D::Line{
                triCenter,
                triCenter + normal * 10
            }.setColor(ColorF32{1.0f, 0.0f, 1.0f}, ColorF32{0.5f, 0, 0.5f})
             .pushAuto();
        Immediate3D::LineSet{}
            .appendTriangle(tri.movedBy(tri.getNormal() * 0.1f))
            .setColor(ColorF32{1.0f, 1.0f, 0.5f})
            .pushAuto();
    }

    // -----------------------------------------------

    struct MachineHit
    {
        float distSqFromStart;
        std::pair<Float3, Float3> closestPair{};
        float distSqOnLineSegment{};
        MachineId otherMachineId;
    };

    std::optional<MachineHit> traceOtherMachineHit(
        const MachinePhysicsState& state,
        const MachinePhysicsProps& props,
        const Float3& fromPos,
        const Float3& toPos)
    {
        const MachineId selfMachineId = props.machineId;
        const float selfRadius = state.m_radius;

        const LineSegment3D selfLine{fromPos, toPos,};
        // const LineSegment3D selfLine{
        //     fromPos - state.m_visualForwardVector * state.m_height * 0.5f,
        //     toPos + state.m_visualForwardVector * state.m_height * 0.5f,
        // };

        float bestDistSq = FLT_MAX;
        std::optional<MachineHit> hit{};
        const auto& otherMachines = GetRaceContext().machineManager().machineList();
        for (int i = 0; i < otherMachines.size(); ++i)
        {
            if (i == selfMachineId)
            {
                continue;
            }

            const auto& other = otherMachines[i];

            if (not other.state.isHitDetectionEnabled())
            {
                continue;
            }

            const Float3& otherPosition = other.state.m_pose.position;
            const Float3& otherForward = other.state.m_visualForwardVector;
            const float otherRadius = other.state.m_radius;
            const LineSegment3D otherLine{
                otherPosition - otherForward * other.state.m_height * 0.5f,
                otherPosition + otherForward * other.state.m_height * 0.5f,
            };

            std::pair<Float3, Float3> closestPair{};
            const float distSqOnLineSegment = DistanceSq(selfLine, otherLine, &closestPair);
            if (distSqOnLineSegment < Math::Square(selfRadius + otherRadius))
            {
                const float distSqFromStart = (otherPosition - fromPos).lengthSq();

                if (distSqFromStart < bestDistSq)
                {
                    bestDistSq = distSqFromStart;
                    hit = MachineHit{
                        .distSqFromStart = distSqFromStart,
                        .closestPair = closestPair,
                        .distSqOnLineSegment = distSqOnLineSegment,
                        .otherMachineId = i,
                    };
                }
            }
        }

        return hit;
    }

    // -----------------------------------------------

    using HitCandidate = Variant<
        StageStaticCollider::ground_hit,
        StageStaticCollider::gimmick_hit,
        MachineHit>;

    Array<HitCandidate> traceHitCandidates(
        const MachinePhysicsState& state,
        const MachinePhysicsProps& props,
        const Float3& fromPos,
        const Float3& toPos,
        bool shouldCheckOtherMachineHit)
    {
        if (fromPos == toPos)
        {
            return {};
        }

        Array<HitCandidate> candidates{};

        {
            const auto moveTestRay = LineSegment3D{fromPos, toPos};
            const auto hitOpt = GetRaceContext().stageManager().stageStaticCollider().rayCastGround(moveTestRay);
            if (hitOpt.has_value())
            {
                candidates.push_back(*hitOpt);
            }
        }

        {
            const auto moveTestCapsule = Capsule3D{fromPos, toPos, state.m_radius};
            const auto hits = GetRaceContext().stageManager().stageStaticCollider().sphereCastGimmick(moveTestCapsule);
            for (const auto& hit : hits)
            {
                candidates.push_back(hit);
            }
        }

        if (shouldCheckOtherMachineHit)
        {
            const auto machineHit = traceOtherMachineHit(state, props, fromPos, toPos);
            if (machineHit.has_value())
            {
                candidates.push_back(*machineHit);
            }
        }

        std::ranges::sort(
            candidates,
            [](const HitCandidate& a, const HitCandidate& b)
            {
                return std::visit([](const auto& hitA, const auto& hitB)
                {
                    return hitA.distSqFromStart < hitB.distSqFromStart;
                }, a, b);
            });

        return candidates;
    }

    // -----------------------------------------------
    // handleGroundHit()

    Float3 bilinear_00_10_01_11(const std::array<Float3, 4>& p, const Float2& uv)
    {
        return p[0] * (1 - uv.x) * (1 - uv.y) +
            p[1] * uv.x * (1 - uv.y) +
            p[2] * (1 - uv.x) * uv.y +
            p[3] * uv.x * uv.y;
    }

    Float3 bilinear_00_10_01_11(const std::array<Float3, 4>& p, float u, float v)
    {
        return bilinear_00_10_01_11(p, Float2{u, v});
    }

    Float2 gaussNewton_00_10_01_11(const std::array<Float3, 4>& p_00_10_01_11, const Float3& p)
    {
        float u = 0.5f, v = 0.5f;
        constexpr int maxIteration = 10;
        for (int iter = 0; iter < maxIteration; ++iter)
        {
            // 現在の点
            Float3 s = bilinear_00_10_01_11(p_00_10_01_11, u, v);

            // 偏微分を数値的に求める (ヤコビアン)
            constexpr float eps = 1e-4f;
            Float3 su = (bilinear_00_10_01_11(p_00_10_01_11, u + eps, v) - s) * (1.0f / eps);
            Float3 sv = (bilinear_00_10_01_11(p_00_10_01_11, u, v + eps) - s) * (1.0f / eps);

            // 残差
            Float3 r = s - p;

            // 2x2 線形方程式を解いて (du, dv) 更新
            float a = su.x * su.x + su.y * su.y + su.z * su.z;
            float b = su.x * sv.x + su.y * sv.y + su.z * sv.z;
            float c = b;
            float d = sv.x * sv.x + sv.y * sv.y + sv.z * sv.z;
            float det = a * d - b * c;
            if (fabs(det) < 1e-8f)
            {
                break;
            }

            // su·r, sv·r
            float ru = su.x * r.x + su.y * r.y + su.z * r.z;
            float rv = sv.x * r.x + sv.y * r.y + sv.z * r.z;

            // Δu, Δv を求める
            float du = (-d * ru + b * rv) / det;
            float dv = (c * ru - a * rv) / det;

            u += du;
            v += dv;

            if (fabs(du) < 1e-5f && fabs(dv) < 1e-5f)
            {
                break;
            }
        }

        return {u, v};
    }

    struct HitSurfaceInfo
    {
        float moveDistance;
        IndexedTriangle tri;
        Float3 normal;
        Float3 surfaceToTriangle;

        void debugDraw() const
        {
            debugDrawTriangle(tri, normal);
        }
    };

    struct SurfacePushback
    {
        Float3 newPos;
        HitSurfaceInfo surface{};
    };

    SurfacePushback pushbackFromSurface(
        const MachinePhysicsState& state,
        const StageStaticCollider::ground_hit& hit,
        const Float3& fromPos,
        const Float3& toPos)
    {
        //         +   T
        //         +  /|
        //         + / |
        //         +/  |
        // U <-- I +   |    <-- normal
        //        /+   |
        //       / +   |
        //      /  +   |
        //     /---+---|
        //     S   H

        const auto& tri = hit.triangle;
        const auto& attr = hit.attribute;

        const Float3 S = fromPos;
        // const Float3 T = toPos;

        // const Float3 H = S - plane.normal * distance;

        Float3 I = hit.hitPosition;

        // const auto bc = tri.getBarycentric(I);

        const std::array<Float3, 4> p_00_10_01_11 =
            TrianglePatternUtil::ArrangePoints_00_10_01_11(tri.p0, tri.p1, tri.p2, attr);
        const Float2 normalUV = gaussNewton_00_10_01_11(p_00_10_01_11, I);

        Float3 normal = bilinear_00_10_01_11(attr.normals_00_10_01_11, normalUV).normalized();
        if (normal.isZero())
        {
            normal = tri.asPlane().normal;
        }

        Float3 bilinearI = bilinear_00_10_01_11(p_00_10_01_11, normalUV);

        const float r = state.m_radius + EPS_CONTACT;

        const Float3 U = bilinearI + normal * r; // TODO

        HitSurfaceInfo hitSurface{};
        hitSurface.tri = tri;
        hitSurface.moveDistance = (U - S).length();
        hitSurface.normal = normal;
        hitSurface.surfaceToTriangle = I - U;
        return {U, hitSurface};
    }

    void onHitGround(
        Float3& newMoveVector,
        MachinePhysicsState& state,
        const Float3& fromPos,
        const Float3& toPos,
        const HitSurfaceInfo& hit)
    {
#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("draw_physics_lines"))
        {
            hit.debugDraw();
        }
#endif

        // 法線の適応
        const Float3 n = hit.normal;

        state.m_surfaceNormal = n;
        state.m_surfaceToTriangle = hit.surfaceToTriangle;

        // 法線方向速度の除去
        state.m_velocity = state.m_velocity - n * n.dot(state.m_velocity);

        // 法線方向移動ベクトルの補正
        const Float3 r = toPos - state.m_pose.position;
        newMoveVector = r - n * r.dot(n);
        if (newMoveVector.isZero())
        {
            return;
        }

        // 移動ベクトルの長さを残りの移動量に調節する
        newMoveVector = newMoveVector.normalized() * Max(0.0f, (toPos - fromPos).length() - hit.moveDistance);
    }

    void handleGroundHit(
        std::optional<Float3>& newMoveVector,
        MachinePhysicsState& state,
        const StageStaticCollider::ground_hit& hit,
        const Float3& fromPos,
        const Float3& toPos)
    {
        auto [newPos, surface] = pushbackFromSurface(state, hit, fromPos, toPos);
        state.m_pose.position = newPos;

        Float3 newMoveVector_{};
        onHitGround(newMoveVector_, state, fromPos, toPos, surface);
        newMoveVector = newMoveVector_;
    }

    // -----------------------------------------------
    // handleGimmickHit()

    struct HitTriInfo
    {
        float moveDistance;
        IndexedTriangle tri;
        Float3 normal;

        void debugDraw() const
        {
            debugDrawTriangle(tri, normal);
        }
    };

    struct TrianglePushback
    {
        Float3 newPos;
        HitTriInfo tri;
    };

    // 衝突した三角形から押し出す (壁に対して用いる)
    TrianglePushback pushbackFromTriangle(
        const MachinePhysicsState& state, const Float3& fromPos, const Float3& toPos, const IndexedTriangle& hit)
    {
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

        const auto plane = hit.asPlane();

        const Float3 S = fromPos;
        const Float3 V = toPos;

        // const Float3 H = S - plane.normal * distance;

        // signedDistance: normal 方向が正
        const float signedDistanceHS = plane.signedDistanceFrom(S);
        const float signedDistanceIH = plane.signedDistanceFrom(V);

        const Float3 SV = V - S;
        const Float3 IV = SV - plane.normal * SV.dot(plane.normal);

        const float r = state.m_radius + EPS_CONTACT;
        const float signedDistanceGS = signedDistanceHS - r;

        if (signedDistanceHS * signedDistanceIH >= 0)
        {
            // 移動ベクトルが面を貫通していない場合
            const Float3 G = S - plane.normal * signedDistanceGS;
            const Float3 T = G + IV;

            HitTriInfo hitTri{};
            hitTri.moveDistance = (T - S).length();
            hitTri.tri = hit;
            hitTri.normal = plane.normal; // TODO: 符号考慮
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

            HitTriInfo hitTri{};
            hitTri.moveDistance = lengthST;
            hitTri.tri = hit;
            hitTri.normal = plane.normal;
            return {T, hitTri};
        }
    }

    void onHitBarrier(
        Float3& newMoveVector,
        MachinePhysicsState& state,
        const Float3& fromPos,
        const Float3& toPos,
        const HitTriInfo& hitTri)
    {
#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("draw_physics_lines"))
        {
            hitTri.debugDraw();
        }
#endif

        state.m_durability = PositiveF32(state.m_durability - 5.0f);

        // state.m_surfaceNormal = {};
        // state.m_surfaceToTriangle = {};

        const Float3 n = hitTri.normal;
        state.m_velocity = state.m_velocity - n * n.dot(state.m_velocity);

        // 法線方向移動ベクトルの補正
        const Float3 r = toPos - state.m_pose.position;
        newMoveVector = r - n * r.dot(n);
        if (newMoveVector.isZero())
        {
            return;
        }

        // 移動ベクトルの長さを残りの移動量に調節する
        newMoveVector =
            newMoveVector.normalized() * Max(0.0f, (toPos - fromPos).length() - hitTri.moveDistance);
    }

    void handleGimmickHit(
        std::optional<Float3>& newMoveVector,
        MachinePhysicsState& state,
        const MachinePhysicsProps& props,
        const StageStaticCollider::gimmick_hit& hit,
        const Float3& fromPos,
        const Float3& toPos)
    {
        switch (hit.attribute.kind)
        {
        case GimmickTriangleAttribute::kind_t::Barrier: {
            // Barrier の押し戻し処理
            auto pushback = pushbackFromTriangle(state, fromPos, toPos, hit.triangle);
            state.m_pose.position = pushback.newPos;

            Float3 newMoveVector_{};
            onHitBarrier(newMoveVector_, state, fromPos, toPos, pushback.tri);
            newMoveVector = newMoveVector_;
            return;
        }
        case GimmickTriangleAttribute::kind_t::BoostPad: {
            // Boost 発生
            state.m_passiveBoost = 1.0f;
            return;
        }
        case GimmickTriangleAttribute::kind_t::JumpPad: {
            if (not state.m_surfaceNormal.isZero())
            {
                // Jump 発生
                state.m_velocity = state.m_velocity - state.m_gravity * state.m_gravity.dot(state.m_velocity);
                state.m_velocity = state.m_velocity - state.m_gravity * 50.0;

                state.m_surfaceNormal = {};
                state.m_surfaceToTriangle = {};
            }

            return;

        case GimmickTriangleAttribute::kind_t::PitZone: {
            // 回復
            if (not state.isDead())
            {
                state.m_durability = PositiveF32(
                    Min<float>(props.maxDurability, state.m_durability + 500.0f * InGameDeltaTime()));
            }

            return;
        }
        }
        default:
            assert(false && "onGimmickHit(): Unsupported GimmickTriangleAttribute::kind_t");
            return;
        }
    }

    // -----------------------------------------------
    // handleOtherMachineHit()

    struct HitMachineInfo
    {
        MachineId otherMachineId{};
        float moveDistance;
        Float3 normal;
        Float3 pushbackVector;
    };

    void onHitOtherMachine(
        Float3& newMoveVector,
        MachinePhysicsState& state,
        const Float3& fromPos,
        const Float3& toPos,
        const HitMachineInfo& hit)
    {
        // 法線の適応
        const Float3 n = hit.normal;

        // NOTE: otherMachine 書き換え 
        auto& otherMachineState = GetRaceContext().machineManager().fetchMachine(hit.otherMachineId).state;

        // 法線方向速度の除去
        const Float3 v1 = state.m_velocity;
        const Float3 v2 = otherMachineState.m_velocity;

        constexpr float m1 = 1;
        constexpr float m2 = m1;
        constexpr float e = 1.0f; // 反発係数

        state.m_velocity = v1 - n * n.dot(v1 - v2) * (1 + e) * m2 / (m1 + m2);
        otherMachineState.m_velocity = v2 - n * n.dot(v2 - v1) * (1 + e) * m1 / (m1 + m2);

        // 法線方向移動ベクトルの補正
        const Float3 r = toPos - state.m_pose.position;
        newMoveVector = r - n * r.dot(n);
        if (newMoveVector.isZero())
        {
            newMoveVector += hit.pushbackVector;
            return;
        }

        // 移動ベクトルの長さを残りの移動量に調節する
        newMoveVector = newMoveVector.normalized() * Max(0.0f, (toPos - fromPos).length() - hit.moveDistance);
        newMoveVector += hit.pushbackVector;
    }

    struct MachinePushback
    {
        Float3 newPos;
        HitMachineInfo hit{};
    };

    MachinePushback pushbackFromMachine(
        const MachinePhysicsState& state,
        const MachineHit& hit,
        const Float3& fromPos,
        const Float3& toPos)
    {
        MachinePushback pushback{};

        const Float3 normal =
            (hit.closestPair.first - hit.closestPair.second).normalized();

        pushback.newPos = hit.closestPair.first;

        pushback.hit.otherMachineId = hit.otherMachineId;

        const auto& otherMachine = GetRaceContext().machineManager().fetchMachine(hit.otherMachineId);
        const float pushbackLength =
            (state.m_radius + otherMachine.state.m_radius) - std::sqrtf(hit.distSqOnLineSegment);
        pushback.hit.pushbackVector = normal * (pushbackLength + EPS_CONTACT);

        // Immediate3D::Line{fromPos, fromPos + normal * 10}.setColor(Palette::White).pushAuto();

        pushback.hit.moveDistance = (pushback.newPos - fromPos).length();
        pushback.hit.normal = normal;

        return pushback;
    }

    void handleOtherMachineHit(
        std::optional<Float3>& newMoveVector,
        MachinePhysicsState& state,
        const MachineHit& hit,
        const Float3& fromPos,
        const Float3& toPos)
    {
        auto [newPos, surface] = pushbackFromMachine(state, hit, fromPos, toPos);
        state.m_pose.position = newPos;

        Float3 newMoveVector_{};
        onHitOtherMachine(newMoveVector_, state, fromPos, toPos, surface);
        newMoveVector = newMoveVector_;
    }

    // -----------------------------------------------

    void resolveMachineMove(
        MachinePhysicsState& state,
        const Float3& moveVector,
        const MachinePhysicsProps& props,
        bool shouldCheckOtherMachineHit,
        int nest)
    {
        constexpr int maxNest = 3;

        if (moveVector.lengthSq() < 1e-6f)
        {
            return;
        }

        const Float3& fromPos = state.m_pose.position;
        const auto toPos = fromPos + moveVector;

        const auto hitCandidates = traceHitCandidates(state, props, fromPos, toPos, shouldCheckOtherMachineHit);
        std::optional<Float3> newMoveVector{};
        for (auto& hit : hitCandidates)
        {
            if (hit.isHolds<StageStaticCollider::ground_hit>())
            {
                handleGroundHit(
                    newMoveVector, state, hit.get<StageStaticCollider::ground_hit>(), fromPos, toPos);
            }
            else if (hit.isHolds<StageStaticCollider::gimmick_hit>())
            {
                handleGimmickHit(
                    newMoveVector, state, props, hit.get<StageStaticCollider::gimmick_hit>(), fromPos, toPos);
            }
            else if (hit.isHolds<MachineHit>())
            {
                handleOtherMachineHit(
                    newMoveVector, state, hit.get<MachineHit>(), fromPos, toPos);
                shouldCheckOtherMachineHit = false;
            }

            // 何らかの物体と接触した場合は newMoveVector を用いて再帰的に更新
            if (newMoveVector.has_value())
            {
                if (nest < maxNest)
                {
                    resolveMachineMove(state, *newMoveVector, props, shouldCheckOtherMachineHit, nest + 1);
                }

                return;
            }
        }

        state.m_pose.position = toPos;
    }

    void resolveMachineGroundContact(MachinePhysicsState& state)
    {
        Float3 vector = state.m_surfaceToTriangle + state.m_surfaceToTriangle.normalized() * state.m_radius;
        if (vector.isZero())
        {
            vector = -state.m_upVector * state.m_radius;
        }

        const Float3 fromPos = state.m_pose.position - vector; // NOTE: 貫通バグ対策で -vector している
        // const Float3 fromPos = state.m_pose.position; // TODO: traceHitCandidates() の壁と地面順序問題を解決したら此方に戻す

        const Float3 toPos = state.m_pose.position + vector;

        const auto moveTestRay = LineSegment3D{fromPos, toPos};
        const auto groundHit = GetRaceContext().stageManager().stageStaticCollider().rayCastGround(moveTestRay);
        if (not groundHit.has_value())
        {
            state.m_surfaceNormal = {};
            return;
        }

        const auto [newPos, hit] = pushbackFromSurface(state, *groundHit, fromPos, toPos);

#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("draw_physics_lines"))
        {
            Immediate3D::Line{fromPos - vector * 10, toPos}.setColor(ColorF32{1.0f, 1, 0}).pushAuto();

            hit.debugDraw();
        }
#endif

        state.m_pose.position = newPos;

        const Float3 n = hit.normal;
        state.m_surfaceNormal = n;

        state.m_surfaceToTriangle = hit.surfaceToTriangle;

        state.m_velocity = state.m_velocity - n * n.dot(state.m_velocity);

        state.m_lastGroundContactLocation = state.m_lapProgress.segmentAndStrip();
    }
}

namespace Race
{
    void ResolveMachineMove(MachinePhysicsState& state, const Float3& moveVector, const MachinePhysicsProps& props)
    {
        resolveMachineMove(state, moveVector, props, true, 0);
    }

    void ResolveMachineGroundContact(MachinePhysicsState& state)
    {
        resolveMachineGroundContact(state);
    }
}
