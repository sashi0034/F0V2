#include "pch.h"
#include "ActorLifetimeScope.h"

#include "ActorBase.h"

namespace TY
{
    ActorLifetimeScope::~ActorLifetimeScope()
    {
        clear();
    }

    ActorLifetimeScope::ActorLifetimeScope(ActorLifetimeScope&& other) noexcept
    {
        m_actorList = std::move(other.m_actorList);
        other.m_actorList.clear();
    }

    void ActorLifetimeScope::append(const std::shared_ptr<ActorBase>& actor)
    {
        assert(actor);
        m_actorList.push_back(actor);
    }

    void ActorLifetimeScope::clear()
    {
        for (auto& actor : m_actorList)
        {
            if (actor->isAlive()) actor->kill();
        }

        m_actorList.clear();
    }

    void ActorLifetimeScope::cleanUp()
    {
        for (int i = static_cast<int>(m_actorList.size()) - 1; i >= 0; --i)
        {
            if (not m_actorList[i]->isAlive())
            {
                m_actorList.erase(m_actorList.begin() + i);
            }
        }
    }

    bool ActorLifetimeScope::anyActive() const
    {
        for (const auto& actor : m_actorList)
        {
            if (actor->isAlive()) return true;
        }

        return false;
    }

    bool ActorLifetimeScope::isEmpty() const
    {
        return m_actorList.empty();
    }
}
