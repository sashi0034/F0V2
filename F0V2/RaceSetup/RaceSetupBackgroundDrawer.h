#pragma once
#include "TY/ActorHandle.h"

namespace RaceSetup
{
    class RaceSetupBackgroundDrawer : public ActorHandle
    {
    public:
        RaceSetupBackgroundDrawer();

        void init();

        void draw() const;

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
