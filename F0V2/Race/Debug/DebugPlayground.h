#pragma once
#include "TY/ActorHandle.h"

namespace Race
{
    class DebugPlayground : public ActorHandle
    {
    public:
        DebugPlayground();

        void init();

        void resetCamera();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
