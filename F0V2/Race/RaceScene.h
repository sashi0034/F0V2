#pragma once
#include "TY/ActorHandle.h"

namespace Race
{
    using namespace TY;

    class RaceScene : public ActorHandle
    {
    public:
        RaceScene();

        void init();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
