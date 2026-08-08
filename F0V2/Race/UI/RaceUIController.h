#pragma once
#include "TY/ActorHandle.h"

namespace Race
{
    struct RaceControlState;

    class RaceUIController : public ActorHandle
    {
    public:
        RaceUIController();

        void init(const RaceControlState& raceControlState);

        std::shared_ptr<ActorBase> asActor() const override;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
