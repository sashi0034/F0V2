#pragma once
#include "MachineEventHandler.h"
#include "MachinePhysicsUnit.h"
#include "TY/Array.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    struct MachineEvaluation
    {
        int rank;
    };

    class MachineManager : public GameObjectHandle
    {
    public:
        MachineManager();

        void init() override;

        MachinePhysicsUnit& fetchMachine(MachineId id);

        const Array<MachinePhysicsUnit>& machineList() const;

        const MachineEvaluation& getEvaluation(MachineId id) const;

        int aliveMachineCount() const;

        MachineEventHandler& eventHandler();
        const MachineEventHandler& eventHandler() const;

        std::shared_ptr<GameObjectBase> asGameObject() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
