#include "pch.h"
#include "Player.h"

#include "Asset.generated.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/GameTime.h"
#include "TY/Intersects3D.h"
#include "TY/KeyboardInput.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/ShapeDrawer.h"
#include "TY/detail/EngineKeyboardMouse.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
    struct Pose
    {
        Float3 position{};
        Quaternion rotation{}; // Euler angles in radians

        Mat4x4 getMatrix() const
        {
            return Mat4x4::Identity()
                   .rotated(rotation)
                   .translated(position);
        }

        Float3 eulerAngles() const
        {
            return rotation.eulerAngles();
        }
    };
}

struct Player::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    float m_radius = 1;
    float m_height = 2;

    Pose m_pose{};
    float m_yaw{};

    Float3 m_surfaceNormal{0, 1, 0};

    void Init()
    {
        ModelBuffer model = ModelBuffer{PrimitiveModel3D::Capsule(m_radius, m_height, ColorF32{0.5f, 0.7f, 1.0f})};

        m_drawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({GetRaceContextContent().cb.lambert});

        m_pose.position = Float3{0, 50.0f, 0};
    }

private:
    void update() override
    {
        m_drawer.uploadWorldMatrix(m_pose.getMatrix()).draw();

        float rotateInput = (KeyA.pressed() ? -1.0f : 0.0f) + (KeyD.pressed() ? 1.0f : 0.0f);

        m_yaw += rotateInput * Math::ToRadians(90.0f) * InGameDeltaTime();
        m_pose.rotation = Quaternion(m_surfaceNormal, m_yaw); // FIXME

        const Float3 forwardVector = m_pose.rotation.rotate(Float3{0, 0, 1});

        GetRaceContextContent().camera.setEyeAndTarget(
            m_pose.position - forwardVector.withY(0.0f).normalized() * 10.0f + Float3{0, 5.0f, 0}, m_pose.position);

        // -----------------------------------------------

        Float3 moveVector{};
        moveVector.y += -InGameDeltaTime() * 5.0f;

        if (KeyUp.pressed())
        {
            moveVector += forwardVector * InGameDeltaTime() * 10.0f;
        }

        updateCapsulePosition(m_pose.position, moveVector);

        ShapeDrawer::Global().draw();

        // -----------------------------------------------

        debugUI();
    }

    void debugUI()
    {
        ImGui::Begin("Player");

        if (ImGui::Button("Reset Position"))
        {
            m_pose.position = Float3{0, 50.0f, 0};
            m_surfaceNormal = Float3{0, 1, 0};
            m_pose.rotation = Quaternion::Identity();
        }

        ImGui::Text("Normal: (%.2f, %.2f, %.2f)", m_surfaceNormal.x, m_surfaceNormal.y, m_surfaceNormal.z);

        ImGui::End();
    }

    // -----------------------------------------------

    void updateCapsulePosition(const Float3& fromPos, const Float3& moveVector, int nest = 0)
    {
        if (moveVector.lengthSq() < 1e-6f)
        {
            return;
        }

        const auto toPos = fromPos + moveVector;
        const auto [newPos, hitTris] = tryMoveCapsulePosition(fromPos, toPos);

        m_pose.position = newPos;

        if ((toPos - newPos).lengthSq() < 1e-6f)
        {
            return;
        }

        if (hitTris.has_value())
        {
            const auto& tri = *hitTris;
            const auto triCenter = tri.tri.centroid();
            Shape3D::Line{
                    triCenter,
                    triCenter + tri.plane.normal * 10
                }.setColor(ColorF32{1.0f, 0.0f, 1.0f}, ColorF32{0.5f, 0, 0.5f})
                 .pushAuto();
            Shape3D::LineSet{}
                .appendTriangle(tri.tri.movedBy(tri.plane.normal * 0.1f))
                .setColor(ColorF32{1.0f, 1.0f, 0.5f})
                .pushAuto();

            // 面の法線を採用
            m_surfaceNormal = tri.plane.normal;

            const auto n = m_surfaceNormal;
            const Float3 r = toPos - m_pose.position;
            const auto newMoveVector = r - n * r.dot(n);
            if (nest < 3)
            {
                updateCapsulePosition(m_pose.position, newMoveVector, nest + 1);
            }
        }
    }

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

    MoveResult tryMoveCapsulePosition(const Float3& fromPos, const Float3& toPos)
    {
        if (fromPos == toPos)
        {
            return {toPos, {}};
        }

        const auto moveTestCapsule = Capsule{fromPos, toPos, m_radius};

        HitTri hitTri{};
        hitTri.moveDistance = FLT_MAX;
        Float3 newPos = toPos;

        const auto hits = GetRaceContext().stageManager().staticBvh().queryHits(moveTestCapsule.aabb());
        hits.forEachTriangle([&](const Triangle3D& tri)
        {
            tryMoveCapsulePosition_internal(tri, moveTestCapsule, fromPos, toPos, hitTri, newPos);
        });

        return {newPos, hitTri};
    }

    inline static int s_triTestCount{};

    void tryMoveCapsulePosition_internal(
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

            const float r = m_radius + 1e-2f;

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

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"Player";
    }
};

namespace Race
{
    Player::Player() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void Player::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> Player::asGameObject() const
    {
        return p_impl;
    }
}
