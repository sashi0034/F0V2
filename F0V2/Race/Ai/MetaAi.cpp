#include "pch.h"
#include "MetaAi.h"

#include "Asset.generated.h"
#include "CharacterAi.h"
#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
    float evaluateRubberBandingBoost(const MachinePhysicsUnit& machine)
    {
        // TODO: rubberBandingBias

        if (machine.state.isHovering())
        {
            return 1.0f;
        }

        const auto& stageManager = GetRaceContext().stageManager();

        const auto& playerMachine = GetRaceContext().machineManager().machineList()[PlayerMachineId];
        const float playerDistance = stageManager.getDistanceFromStart(playerMachine.state.m_lapProgress);

        const float thisDistance = stageManager.getDistanceFromStart(machine.state.m_lapProgress);

        const float distanceFromPlayer = thisDistance - playerDistance;

        float r = -distanceFromPlayer / 100.0f;
        r = Math::Clamp(r, -1.0f, 1.0f); // [-1.0f, 1.0f]
        // r = (r + 1.0f) * 0.5f; // [-1.0f, 1.0f] --> [0.0f, 1.0f]

        float boostFactor;
        if (r < 0.0f)
        {
            boostFactor = 1.0f + r * 0.5f;
        }
        else // if (r >= 0.0f)
        {
            boostFactor = 1.0f + r * 4.0f;
        }

        return boostFactor;
    }
}

struct MetaAi::Impl : GameObjectBase
{
    ActorContainer m_children{};

    void Init()
    {
    }

private:
    void update() override
    {
        auto& characterAiList = GetRaceContext().characterAiList();
        for (int i = 0; i < characterAiList.size(); i++)
        {
            const auto& aiMachine = GetRaceContext().machineManager().machineList()[characterAiList[i].machineId()];

            CharacterAiInputCommand command{};
            command.targeCheatBoost = evaluateRubberBandingBoost(aiMachine);

            characterAiList[i].setInputCommand(command);
        }
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"MetaAi";
    }
};

namespace Race
{
    MetaAi::MetaAi() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void MetaAi::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> MetaAi::asGameObject() const
    {
        return p_impl;
    }
}
