#include "pch.h"
#include "ActorLifetimeScope.h"

#include "ActorBase.h"

namespace Util
{
	ActorLifetimeScope::~ActorLifetimeScope()
	{
		Clear();
	}

	ActorLifetimeScope::ActorLifetimeScope(ActorLifetimeScope&& other) noexcept
	{
		m_actorList = std::move(other.m_actorList);
		other.m_actorList.clear();
	}

	void ActorLifetimeScope::Append(const std::shared_ptr<ActorBase>& actor)
	{
		assert(actor);
		m_actorList.push_back(actor);
	}

	void ActorLifetimeScope::Clear()
	{
		for (auto& actor : m_actorList)
		{
			if (actor->IsAlive()) actor->Kill();
		}

		m_actorList.clear();
	}

	void ActorLifetimeScope::CleanUp()
	{
		for (int i = static_cast<int>(m_actorList.size()) - 1; i >= 0; --i)
		{
			if (not m_actorList[i]->IsAlive())
			{
				m_actorList.erase(m_actorList.begin() + i);
			}
		}
	}

	bool ActorLifetimeScope::AnyActive() const
	{
		for (const auto& actor : m_actorList)
		{
			if (actor->IsAlive()) return true;
		}

		return false;
	}

	bool ActorLifetimeScope::IsEmpty() const
	{
		return m_actorList.empty();
	}
}
