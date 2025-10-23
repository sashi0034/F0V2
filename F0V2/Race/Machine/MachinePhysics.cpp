#include "pch.h"
#include "MachinePhysics.h"

#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"
#include "TY/GameStep.h"
#include "TY/GameTime.h"
#include "TY/Intersects3D.h"
#include "TY/Immediate3D.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

namespace
{
    constexpr float epsGround = 1e-2f;

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

    struct HitSurface
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

    struct MoveResult
    {
        Float3 newPos;
        std::optional<HitSurface> hit{};
    };

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

    MoveResult moveOnGround(const MachinePhysicsState& state, const Float3& fromPos, const Float3& toPos)
    {
        if (fromPos == toPos)
        {
            return {toPos, std::nullopt};
        }

        // const auto moveTestCapsule = Capsule{fromPos, toPos, state.m_radius}; // FIXME?
        const auto moveTestRay = LineSegment3D{fromPos, toPos};

        const auto hitOpt = GetRaceContext().stageManager().stageStaticCollider().rayCastGround(moveTestRay);
        if (not hitOpt.has_value())
        {
            // 衝突なし
            return {toPos, std::nullopt};
        }

        // -----------------------------------------------
        // 衝突あり

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

        const auto& tri = hitOpt.value().triangle;
        const auto& attr = hitOpt.value().attribute;

        const Float3 S = fromPos;
        // const Float3 T = toPos;

        // const Float3 H = S - plane.normal * distance;

        Float3 I = hitOpt.value().hitPosition;

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

        const float r = state.m_radius + epsGround;

        const Float3 U = bilinearI + normal * r; // TODO

        HitSurface hitTri{};
        hitTri.tri = tri;
        hitTri.moveDistance = (U - S).length();
        hitTri.normal = normal;
        hitTri.surfaceToTriangle = I - U;
        return {U, hitTri};
    }

    // -----------------------------------------------

    struct HitTri
    {
        float moveDistance;
        IndexedTriangle tri;
        Float3 normal;

        void debugDraw() const
        {
            debugDrawTriangle(tri, normal);
        }
    };

    struct PushbackResult
    {
        Float3 newPos;
        HitTri hitTri;
    };

    // 衝突した三角形から押し出す (壁に対して用いる)
    PushbackResult pushbackFromTriangle(
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

        const float r = state.m_radius + epsGround;
        const float signedDistanceGS = signedDistanceHS - r;

        if (signedDistanceHS * signedDistanceIH >= 0)
        {
            // 移動ベクトルが面を貫通していない場合
            const Float3 G = S - plane.normal * signedDistanceGS;
            const Float3 T = G + IV;

            HitTri hitTri{};
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

            HitTri hitTri{};
            hitTri.moveDistance = lengthST;
            hitTri.tri = hit;
            hitTri.normal = plane.normal;
            return {T, hitTri};
        }
    }

    struct GimmickResult
    {
        std::optional<PushbackResult> pushback{};
        bool boostPad{};
        bool jumpPad{};
    };

    GimmickResult checkGimmickHit(
        const MachinePhysicsState& state,
        const Float3& fromPos,
        const Float3& toPos)
    {
        if (fromPos == toPos)
        {
            return {};
        }

        const auto moveTestCapsule = Capsule3D{fromPos, toPos, state.m_radius};

        GimmickResult result{};
        const auto hits = GetRaceContext().stageManager().stageStaticCollider().sphereCastGimmick(moveTestCapsule);
        for (const auto& hit : hits)
        {
            switch (hit.attribute.kind)
            {
            case GimmickTriangleAttribute::kind_t::Barrier: {
                result.pushback = pushbackFromTriangle(state, fromPos, toPos, hit.triangle);
                return result;
            }
            case GimmickTriangleAttribute::kind_t::BoostPad: {
                result.boostPad = true;
                break;
            }
            case GimmickTriangleAttribute::kind_t::JumpPad: {
                result.jumpPad = true;
                break;
            }
            default:
                assert(false && "tryMoveGimmickPosition(): Unsupported gimmick triangle kind");
            }
        }

        return result;
    }

    // -----------------------------------------------

    void onHitGround(
        Float3& newMoveVector,
        MachinePhysicsState& state,
        const MachinePhysicsProps& props,
        const Float3& moveVector,
        const Float3& fromPos,
        const HitSurface& hit)
    {
        if (props.debug.drawHitTris)
        {
            hit.debugDraw();
        }

        // 法線の適応
        const Float3 n = hit.normal;

        state.m_surfaceNormal = n;
        state.m_surfaceToTriangle = hit.surfaceToTriangle;

        // 法線方向速度の除去
        state.m_velocity = state.m_velocity - n * n.dot(state.m_velocity);

        // 法線方向移動ベクトルの補正
        const Float3 toPos = fromPos + moveVector;
        const Float3 r = toPos - state.m_pose.position;
        newMoveVector = r - n * r.dot(n);
        if (newMoveVector.isZero())
        {
            return;
        }

        // 移動ベクトルの長さを残りの移動量に調節する
        newMoveVector = newMoveVector.normalized() * Max(0.0f, moveVector.length() - hit.moveDistance);
    }

    void onHitBarrier(
        Float3& newMoveVector,
        MachinePhysicsState& state,
        const MachinePhysicsProps& props,
        const Float3& moveVector,
        const Float3& fromPos,
        const HitTri& hitTri)
    {
        if (props.debug.drawHitTris)
        {
            hitTri.debugDraw();
        }

        // state.m_surfaceNormal = {};
        // state.m_surfaceToTriangle = {};

        const Float3 n = hitTri.normal;
        state.m_velocity = state.m_velocity - n * n.dot(state.m_velocity);

        // 法線方向移動ベクトルの補正
        const Float3 toPos = fromPos + moveVector;
        const Float3 r = toPos - state.m_pose.position;
        newMoveVector = r - n * r.dot(n);
        if (newMoveVector.isZero())
        {
            return;
        }

        // 移動ベクトルの長さを残りの移動量に調節する
        newMoveVector =
            newMoveVector.normalized() * Max(0.0f, moveVector.length() - hitTri.moveDistance);
    }

    void updateCapsulePosition(
        MachinePhysicsState& state,
        const MachinePhysicsProps& props,
        const Float3& fromPos,
        const Float3& moveVector,
        int nest = 0)
    {
        constexpr int maxNest = 3;

        if (moveVector.lengthSq() < 1e-6f)
        {
            return;
        }

        const auto toPos = fromPos + moveVector;
        const auto [newPos, hitOpt] = moveOnGround(state, fromPos, toPos);

        // ギミックの対応
        {
            const Float3 resultMoveVector = newPos - fromPos;
            const Float3 moveVector2 = moveVector * resultMoveVector.dot(moveVector) / moveVector.lengthSq();

            const auto gimmickResult = checkGimmickHit(state, fromPos, fromPos + moveVector2);

            if (gimmickResult.boostPad > 0.0f)
            {
                // Boost 発生
                state.m_additionalBoost = 1.0f;
            }

            if (gimmickResult.jumpPad > 0.0f)
            {
                // Jump 発生
                state.m_velocity = -state.m_gravity * 50.0; // TODO: 現在の速度に応じてジャンプ量を決める

                state.m_surfaceNormal = {};
                state.m_surfaceToTriangle = {};
            }

            if (gimmickResult.pushback.has_value())
            {
                // Barrier の押し戻し処理
                const auto& pushback = *gimmickResult.pushback;
                state.m_pose.position = pushback.newPos;

                Float3 newMoveVector{};
                onHitBarrier(newMoveVector,
                             state,
                             props,
                             moveVector,
                             fromPos,
                             pushback.hitTri);
                if (nest < maxNest)
                {
                    updateCapsulePosition(state, props, state.m_pose.position, newMoveVector, nest + 1);
                    return;
                }
            }
        }

        state.m_pose.position = newPos;

        if (hitOpt.has_value())
        {
            // Ground の押し戻し処理
            const auto& hitSurface = *hitOpt;

            Float3 newMoveVector{};
            onHitGround(newMoveVector,
                        state,
                        props,
                        moveVector,
                        fromPos,
                        hitSurface);
            if (nest < maxNest)
            {
                updateCapsulePosition(state, props, state.m_pose.position, newMoveVector, nest + 1);
                return;
            }
        }
    }

    // -----------------------------------------------

    void updateGroundedness(MachinePhysicsState& state)
    {
        state.m_upVector = state.m_surfaceNormal;
        if (state.m_upVector.isZero())
        {
            state.m_upVector = -state.m_gravity;
        }

        Float3 vector = state.m_surfaceToTriangle + state.m_surfaceToTriangle.normalized() * state.m_radius;
        if (vector.isZero())
        {
            vector = -state.m_upVector * state.m_radius;
        }

        const Float3 fromPos = state.m_pose.position;
        const Float3 toPos = state.m_pose.position + vector;

        const auto [newPos, hitOpt] = moveOnGround(state, fromPos, toPos);
        if (hitOpt.has_value())
        {
            Immediate3D::Line{fromPos - vector * 10, toPos}.setColor(ColorF32{1.0f, 1, 0}).pushAuto();

            const auto& hit = *hitOpt;

            hit.debugDraw();

            state.m_pose.position = newPos;

            const Float3 n = hit.normal;
            state.m_surfaceNormal = n;

            state.m_surfaceToTriangle = hit.surfaceToTriangle;

            state.m_velocity = state.m_velocity - n * n.dot(state.m_velocity);
        }
        else
        {
            state.m_surfaceNormal = {};
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

    Float3 calculateGravity(const MachinePhysicsState& state, const SegmentAndStrip& nearestSegmentAndStrip)
    {
        const Float3& position = state.m_pose.position;

        const auto& courseSegments = GetRaceContext().stageManager().courseSegments();

        const auto& nearestSegment = courseSegments[nearestSegmentAndStrip.segmentIndex];

        const auto& nearestStrip = nearestSegment.midwayStrips[nearestSegmentAndStrip.stripIndex];

        if (nearestStrip.style == CourseSegmentStyle::Road)
        {
            return -nearestStrip.normal;
        }
        else if (nearestStrip.style == CourseSegmentStyle::Pipe)
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

        assert(false);
        return {};
    }
}

namespace Race
{
    Float3 MachinePhysicsState::rightVector() const
    {
        return m_upVector.cross(m_forwardVector).normalized();
    }

    void UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props)
    {
        // const Float3 gravity = state.m_gravity - state.m_surfaceNormal * state.m_surfaceNormal.dot(state.m_gravity);
        Float3 gravity = state.m_gravity; // TODO: 地面方向の成分を除去
        state.m_velocity += gravity * 50.0f * InGameDeltaTime();

        if (props.hasAccelInput)
        {
            state.m_velocity += state.m_forwardVector * 50.0f * InGameDeltaTime();
        }

        const float maxSpeed = 100.0f;
        if (state.m_velocity.lengthSq() > Math::Square(maxSpeed))
        {
            state.m_velocity = state.m_velocity.normalized() * maxSpeed;
        }

        // ブースト処理
        if (state.m_additionalBoost > 0.0f)
        {
            state.m_velocity += state.m_forwardVector * 100.0f * Min(1.0f, state.m_additionalBoost) * InGameDeltaTime();

            state.m_additionalBoost = Max<float>(0.0f, state.m_additionalBoost - InGameDeltaTime());
        }

        // 移動処理
        {
            Float3 moveVector = state.m_velocity * InGameDeltaTime();

            // moveVector += -state.m_upVector * gravity * 10.0f * InGameDeltaTime(); // 常に微小量の力で地面方向に押し付ける

            updateCapsulePosition(state, props, state.m_pose.position, moveVector);
        }

        // ImmediatePrint(std::format("groundedness: {:.2f}", state.m_groundedness), Alignment9::BottomCenter);

        const auto& courseSegments = GetRaceContext().stageManager().courseSegments();

        const auto nearestSegmentAndStrip = findNearestSegmentAndStrip(courseSegments, state.m_pose.position);

        {
            state.m_lapProgress = EvaluateLapProgress(state.m_lapProgress, nearestSegmentAndStrip);

            ImmediatePrint(
                std::format("Lap: {}, Segment: {}, Strip: {}",
                            state.m_lapProgress.lapIndex,
                            state.m_lapProgress.segmentIndex,
                            state.m_lapProgress.stripIndex),
                Alignment9::TopCenter);
        }

        // 現在位置における重力方向を計算
        {
            state.m_gravity = calculateGravity(state, nearestSegmentAndStrip);

#if 0
            state.m_gravity = Float3(0, -1, 0);
#endif

            if (props.debug.drawHitTris)
            {
                Immediate3D::Line{
                        state.m_pose.position,
                        state.m_pose.position - state.m_gravity * 10
                    }.setColor(ColorF32{0.3f, 0.0f, 0.3f}, ColorF32{0.1f, 0, 0.1f})
                     .pushAuto();
            }
        }

        // -----------------------------------------------

        state.m_forwardVector = state.m_forwardVector - state.m_upVector * state.m_upVector.dot(state.m_forwardVector);

        if (state.m_forwardVector.isZero())
        {
            state.m_forwardVector = Mat4x4{state.m_pose.rotation}.forward();
            state.m_forwardVector = state.m_forwardVector - state.m_upVector * state.m_upVector.dot(
                state.m_forwardVector);

            if (state.m_forwardVector.isZero())
            {
                assert(false); // TODO
            }
        }

        state.m_forwardVector = state.m_forwardVector.normalized();

        if (state.m_additionalBoost > 0.0f)
        {
            ImmediatePrint("<<< BOOST >>>", Alignment9::MiddleCenter);
        }

        ImmediatePrint(
            std::format("Pos: {:.02f}, {:.02f}, {:.02f}", state.m_pose.position.x, state.m_pose.position.y,
                        state.m_pose.position.z), Alignment9::MiddleCenter);

        ImmediatePrint(
            std::format("Forward: {:.02f}, {:.02f}, {:.02f}",
                        state.m_forwardVector.x, state.m_forwardVector.y, state.m_forwardVector.z),
            Alignment9::MiddleCenter);

        ImmediatePrint(
            std::format("Up: {:.02f}, {:.02f}, {:.02f}", state.m_upVector.x, state.m_upVector.y, state.m_upVector.z),
            Alignment9::MiddleCenter);

        ImmediatePrint(
            std::format("Gravity: {:.02f}, {:.02f}, {:.02f}", state.m_gravity.x, state.m_gravity.y, state.m_gravity.z),
            Alignment9::MiddleCenter
        );

        const Quaternion targetRotation =
            Quaternion::FromUnitVectors(Float3{0, 1, 0}, state.m_upVector); // TODO: pitch

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
