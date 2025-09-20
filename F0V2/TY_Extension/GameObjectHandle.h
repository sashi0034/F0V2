#pragma once
#include "GameObjectBase.h"
#include "TY/ActorBase.h"
#include "TY/ActorHandle.h"

namespace TY
{
    class GameObjectHandle : public ActorHandle
    {
    public:
        std::shared_ptr<ActorBase> asActor() const override
        {
            return asGameObject();
        }

        virtual std::shared_ptr<GameObjectBase> asGameObject() const = 0;

        virtual void init();
    };
}
