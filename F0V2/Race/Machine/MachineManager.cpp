#include "pch.h"
#include "MachineManager.h"

#include "MachineEventHandler.h"
#include "MachinePhysicsUnit.h"
#include "TY/ActorContainer.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
}

struct MachineManager::Impl : GameObjectBase
{
    bool m_initialized{};

    ActorContainer m_children{};

    MachineEventHandler m_eventHandler{};

    Array<MachinePhysicsUnit> m_physicsUnits{};

    Array<MachineEvaluation> m_evaluations{};

    void Init()
    {
        m_initialized = true;

        m_physicsUnits.reserve(100);

        m_eventHandler = m_children.birth(MachineEventHandler());
        m_eventHandler.init();
    }

    void ResizeIfNeeded(MachineId id)
    {
        assert(m_initialized);

        while (m_physicsUnits.size() <= id)
        {
            const int nextid = m_physicsUnits.size();
            m_physicsUnits.push_back({});
            m_physicsUnits.back().props.machineId = nextid;

            m_evaluations.push_back(MachineEvaluation{
                .rank = nextid,
            });
        }
    }

private:
    void update() override
    {
        m_children.updateEach();

        evaluateMachines();
    }

    void evaluateMachines()
    {
        Array<std::pair<int, LapProgress>> progressList;
        for (int i = 0; i < m_physicsUnits.size(); ++i)
        {
            const auto& machine = m_physicsUnits[i];
            progressList.push_back({i, machine.state.m_lapProgress});
        }

        // m_lapProgress が大きい順にソート
        std::ranges::sort(
            progressList,
            [](const auto& a, const auto& b)
            {
                return b.second.isLessThan(a.second);
            });

        m_evaluations.resize(progressList.size());
        for (int rank = 0; rank < progressList.size(); ++rank)
        {
            const int index = progressList[rank].first;
            m_evaluations[index] = MachineEvaluation{
                .rank = rank,
            };
        }
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
        p_impl->ResizeIfNeeded(id);
        return p_impl->m_physicsUnits[id];
    }

    const MachineEvaluation& MachineManager::getEvaluation(MachineId id) const
    {
        p_impl->ResizeIfNeeded(id);
        return p_impl->m_evaluations[id];
    }

    const Array<MachinePhysicsUnit>& MachineManager::machineList() const
    {
        return p_impl->m_physicsUnits;
    }

    MachineEventHandler& MachineManager::eventHandler()
    {
        return p_impl->m_eventHandler;
    }

    const MachineEventHandler& MachineManager::eventHandler() const
    {
        return p_impl->m_eventHandler;
    }

    std::shared_ptr<GameObjectBase> MachineManager::asGameObject() const
    {
        return p_impl;
    }
}
