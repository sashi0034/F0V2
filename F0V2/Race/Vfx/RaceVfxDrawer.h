#pragma once

#include "IRaceVfxSystem.h"
#include "TY/ActorHandle.h"

namespace Race
{
    class RaceVfxDrawer : public ActorHandle
    {
    public:
        RaceVfxDrawer();

        void init();

        void registerVfxSystem(const std::shared_ptr<IRaceVfxSystem>& system);

        void unregisterVfxSystem(const IRaceVfxSystem* system);

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
