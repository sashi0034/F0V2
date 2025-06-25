#pragma once
#include "ActorBase.h"

namespace TY
{
    class ActorHandle
    {
    public:
        virtual ~ActorHandle() = default;

        bool isAlive() const;

        void kill();

        virtual std::shared_ptr<ActorBase> asActor() const = 0;

        operator ActorWeakRef() const;
    };
}
