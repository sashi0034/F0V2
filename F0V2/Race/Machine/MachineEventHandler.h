#pragma once
#include "MachinePhysics.h"
#include "TY/ActorHandle.h"

namespace Race
{
    class MachineEventHandler : public ActorHandle
    {
    public:
        MachineEventHandler();

        void init();

        void handleIfNeeded(MachineId id);

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
