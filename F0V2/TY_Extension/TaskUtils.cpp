#include "pch.h"
#include "TaskUtils.h"

namespace TY
{
    std::function<bool()> MakeTimeoutTask(float seconds, float (*deltaTime)())
    {
        const auto timer = std::make_shared<float>(seconds);
        return [timer , deltaTime]
        {
            *timer -= deltaTime();
            return *timer <= 0.0;
        };
    }

    std::function<bool()> MakeTimeoutTask(Duration seconds, float (*deltaTime)())
    {
        return MakeTimeoutTask(seconds.count(), deltaTime);
    }

    std::function<bool()> MakeExpireObserver(ActorWeakRef actor)
    {
        return [actor]
        {
            const auto lock = actor.lock();
            return lock && not lock->isAlive();
        };
    }
}
