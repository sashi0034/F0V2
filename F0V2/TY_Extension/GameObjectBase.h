#pragma once

#include "TY/ActorBase.h"

namespace TY
{
    class GameObjectBase : public ActorBase
    {
    public:
        virtual std::u32string name() const = 0;

        virtual void debugInspector() { return; }
    };
}
