#pragma once

#include "TY/ActorHandle.h"

namespace Race
{
    class MachineVfxEmitter : public ActorHandle
    {
    public:
        MachineVfxEmitter();

        void init();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
