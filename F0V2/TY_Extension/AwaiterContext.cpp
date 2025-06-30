#include "pch.h"
#include "AwaiterContext.h"

using namespace TY;

namespace TY
{
    AwaiterContext::AwaiterContext(std::reference_wrapper<CoroutineActor::yield_type> yield) :
        m_yield(yield)
    {
    }

    void AwaiterContext::waitForFrames(int frames)
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

    void AwaiterContext::waitForTime(double seconds, std::function<double()> deltaTime)
    {
        m_resumePoller = [&seconds, deltaTime]() -> bool
        {
            seconds -= deltaTime();
            return seconds <= 0.0;
        };

        yield();
    }

    void AwaiterContext::waitForTime(Duration seconds, const std::function<double()>& deltaTime)
    {
        waitForTime(seconds.count(), deltaTime);
    }

    void AwaiterContext::waitForever()
    {
        waitForTrue([]() { return false; });
    }

    void AwaiterContext::waitForTrue(const std::function<bool()>& poller)
    {
        if (poller != nullptr && poller()) return;

        m_resumePoller = poller;

        yield();
    }

    int AwaiterContext::waitAnyTrue(const Array<std::function<bool()>>& pollers)
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

    void AwaiterContext::waitForExpired(const ActorHandle& actor)
    {
        waitForExpired(actor.asActor());
    }

    void AwaiterContext::waitForExpired(std::shared_ptr<ActorBase> actor)
    {
        const std::weak_ptr weakRef = actor;
        waitForExpired(weakRef);
    }

    void AwaiterContext::waitForExpired(std::weak_ptr<ActorBase> actor)
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
        CoroutineActor::yield_type& y = m_yield.get();
        y();
    }

    bool AwaiterController::validateResume()
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
