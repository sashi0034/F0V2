#pragma once
#include "CharacterAIInputCommand.h"
#include "Race/Machine/MachinePhysicsUnit.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class CharacterAI : public ActorHandle
    {
    public:
        CharacterAI();

        void init(int aiId);

        MachineId machineId() const;

        void setInputCommand(const CharacterAIInputCommand& command);

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
