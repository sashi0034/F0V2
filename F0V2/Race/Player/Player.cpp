#include "pch.h"
#include "Player.h"

#include "Asset.generated.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextState.h"
#include "TY/ActorContainer.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
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

    void Init()
    {
        ModelBuffer model = ModelBuffer{PrimitiveModel3D::Capsule(m_radius, m_height, ColorF32{0.5f, 0.7f, 1.0f})};

        m_drawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({GetRaceContextState().cb.lambert});

        m_pose.position = Float3{0, 50.0f, 0};
    }

private:
    void update() override
    {
        m_drawer.uploadWorldMatrix(Mat4x4::Translate(m_pose.position)).draw();

        GetRaceContextState().camera.setEyeAndTarget(
            m_pose.position + Float3{0, 5, -10}, m_pose.position + Float3{0, 2, 0});

        m_pose.position.y += System::DeltaTime() * 5.0f;
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
