#pragma once
#include "Util/ActorHandle.h"

namespace Combat
{
    using namespace Util;

    class CombatScene : public ActorHandle
    {
    public:
        CombatScene();

        std::shared_ptr<ActorBase> AsActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
