#pragma once
#include "TY/ActorHandle.h"

namespace Race
{
    class Hud_DurabilityBar : public ActorHandle
    {
    public:
        Hud_DurabilityBar();

        void init();

        void draw() const;

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
