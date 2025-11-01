#pragma once
#include "Race/Machine/MachinePhysicsUnit.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class CharacterAi : public ActorHandle
    {
    public:
        CharacterAi();

        void init(int aiId);

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
