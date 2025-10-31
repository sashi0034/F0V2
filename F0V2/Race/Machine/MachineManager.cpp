#include "pch.h"
#include "MachineManager.h"

#include "Asset.generated.h"
#include "MachinePhysicsUnit.h"
#include "TY/ActorContainer.h"
#include "TY/ModelDrawer.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
}

struct MachineManager::Impl : GameObjectBase
{
    bool m_initialized{};

    ActorContainer m_children{};

    Array<MachinePhysicsUnit> m_physicsUnits{};

    void Init()
    {
        m_initialized = true;

        m_physicsUnits.reserve(100);
    }

    MachinePhysicsUnit& FetchMachine(MachineId id)
    {
        assert(m_initialized);

        while (m_physicsUnits.size() <= id)
        {
            const int nextid = m_physicsUnits.size();
            m_physicsUnits.push_back({});
            m_physicsUnits.back().props.machineId = nextid;
        }

        return m_physicsUnits[id];
    }

private:
    void update() override
    {
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"MachineManager";
    }
};

namespace Race
{
    MachineManager::MachineManager() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void MachineManager::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    MachinePhysicsUnit& MachineManager::fetchMachine(MachineId id)
    {
        return p_impl->FetchMachine(id);
    }

    Array<MachinePhysicsUnit>& MachineManager::machineList() const
    {
        return p_impl->m_physicsUnits;
    }

    std::shared_ptr<GameObjectBase> MachineManager::asGameObject() const
    {
        return p_impl;
    }
}
