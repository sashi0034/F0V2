#pragma once
#include "MachinePhysicsUnit.h"
#include "TY/Array.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class MachineManager : public GameObjectHandle
    {
    public:
        MachineManager();

        void init() override;

        MachinePhysicsUnit& fetchMachine(MachineId id);

        Array<MachinePhysicsUnit>& machineList() const;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
