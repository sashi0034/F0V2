#pragma once

#include "TY/ActorHandle.h"

namespace Race
{
    class RaceCameraController : public ActorHandle
    {
    public:
        RaceCameraController();

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
