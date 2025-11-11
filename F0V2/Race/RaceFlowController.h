#pragma once
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class RaceFlowController : public ActorHandle
    {
    public:
        RaceFlowController();

        void init();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
