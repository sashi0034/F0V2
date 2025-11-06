#pragma once
#include "IRaceDrawer.h"
#include "TY_Extension/GameObjectHandle.h"

namespace Race
{
    class RaceDrawManager : public ActorHandle
    {
    public:
        RaceDrawManager();

        void init();

        void registerDrawer(const std::shared_ptr<IRaceDrawer>& drawer);

        void unregisterDrawer(const IRaceDrawer* drawer);

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
