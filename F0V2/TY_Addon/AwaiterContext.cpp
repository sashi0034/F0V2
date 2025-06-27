#include "pch.h"
#include "AwaiterContext.h"

using namespace TY;

namespace TY
{
    AwaiterContext::AwaiterContext(std::reference_wrapper<CoroActor::yield_type> yield) :
        m_yield(yield)
    {
    }

    void AwaiterContext::WaitForFrames(int frames)
    {
        m_resumePoller = [&frames]() -> bool
        {
            if (frames <= 0)
            {
                return true;
            }

            --frames;
            return false;
        };

        yield();
    }

    void AwaiterContext::WaitForTime(double seconds, std::function<double()> deltaTime)
    {
        m_resumePoller = [&seconds, deltaTime]() -> bool
        {
            seconds -= deltaTime();
            return seconds <= 0.0;
        };

        yield();
    }

    // void AwaiterContext::WaitForTime(Duration seconds, const std::function<double()>& deltaTime)
    // {
    //     WaitForTime(seconds.count(), deltaTime);
    // }

    void AwaiterContext::WaitForever()
    {
        WaitForTrue([]() { return false; });
    }

    void AwaiterContext::WaitForTrue(const std::function<bool()>& poller)
    {
        if (poller != nullptr && poller()) return;

        m_resumePoller = poller;

        yield();
    }

    int AwaiterContext::WaitAnyTrue(const Array<std::function<bool()>>& pollers)
    {
        for (int i = 0; i < pollers.size(); ++i)
        {
            if (pollers[i] != nullptr && pollers[i]()) return i;
        }

        int result{};
        m_resumePoller = [&]() -> bool
        {
            for (int i = 0; i < pollers.size(); ++i)
            {
                if (pollers[i] != nullptr && pollers[i]())
                {
                    result = i;
                    return true;
                }
            }

            return false;
        };

        yield();
        return result;
    }

    void AwaiterContext::WaitForExpired(const ActorHandle& actor)
    {
        WaitForExpired(actor.asActor());
    }

    void AwaiterContext::WaitForExpired(std::shared_ptr<ActorBase> actor)
    {
        const std::weak_ptr weakRef = actor;
        WaitForExpired(weakRef);
    }

    void AwaiterContext::WaitForExpired(std::weak_ptr<ActorBase> actor)
    {
        const auto actorObject = actor.lock();
        if (not actorObject) return;
        if (not actorObject->isAlive()) return;

        m_resumePoller = [actor]() -> bool
        {
            const auto actorObject = actor.lock();
            return not actorObject || not actorObject->isAlive();
        };

        yield();
    }

    void AwaiterContext::yield()
    {
        CoroActor::yield_type& y = m_yield.get();
        y();
    }

    bool AwaiterController::ValidateResume()
    {
        if (not m_resumePoller) return true;

        // m_resumeController を 1 フレーム分進め、再開可能であるかを調べる
        if (m_resumePoller())
        {
            m_resumePoller = {};
            return true;
        }

        return false;
    }
}
