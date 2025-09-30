#include "pch.h"
#include "Player.h"

#include "Asset.generated.h"
#include "TY/ActorContainer.h"
#include "TY/ModelDrawer.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
}

struct Player::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    void Init()
    {
        m_drawer =
            ModelDrawerParams{}
            .setModel(Asset_model::cinnamon)
            .setShader(Asset_shader::model);
    }

private:
    void update() override
    {
        m_drawer.uploadWorldMatrix(Mat4x4::Identity()).draw();
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
