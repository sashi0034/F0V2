#pragma once
#include "TY/ActorHandle.h"

namespace Combat
{
    using namespace TY;

    class CombatScene : public ActorHandle
    {
    public:
        CombatScene();

        void init();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
