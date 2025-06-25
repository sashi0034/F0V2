#include "pch.h"
#include "ActorHandle.h"

namespace Util
{
	bool ActorHandle::IsAlive() const
	{
		const auto& actor = AsActor();
		if (not actor) return false;
		return actor->IsAlive();
	}

	void ActorHandle::Kill()
	{
		if (const auto& actor = AsActor())
		{
			actor->Kill();
		}
	}

	ActorHandle::operator std::weak_ptr<ActorBase>() const
	{
		return AsActor();
	}
}
