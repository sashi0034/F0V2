#pragma once

#include "IRaceEffectSystem.h"
#include "TY/ActorHandle.h"

namespace Race
{
    class RaceEffectDrawer : public ActorHandle
    {
    public:
        RaceEffectDrawer();

        void init();

        void registerEffectSystem(const std::shared_ptr<IRaceEffectSystem>& system);

        void unregisterEffectSystem(const IRaceEffectSystem* system);

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
