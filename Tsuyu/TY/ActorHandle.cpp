#include "pch.h"
#include "ActorHandle.h"

namespace TY
{
	bool ActorHandle::isAlive() const
	{
		const auto& actor = asActor();
		if (not actor) return false;
		return actor->isAlive();
	}

	void ActorHandle::kill()
	{
		if (const auto& actor = asActor())
		{
			actor->kill();
		}
	}

	ActorHandle::operator std::weak_ptr<ActorBase>() const
	{
		return asActor();
	}
}
