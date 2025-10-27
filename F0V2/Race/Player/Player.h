#pragma once
#include "Race/Machine/MachinePhysicsUnit.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class Player : public GameObjectHandle
    {
    public:
        Player();

        void init() override;

        const MachinePhysicsUnit& machine() const;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
