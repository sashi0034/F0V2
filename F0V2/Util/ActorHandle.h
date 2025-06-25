#pragma once
#include "ActorBase.h"

namespace Util
{
	class ActorHandle
	{
	public:
		virtual ~ActorHandle() = default;

		bool IsAlive() const;

		void Kill();

		virtual std::shared_ptr<ActorBase> AsActor() const = 0;

		operator ActorWeakRef() const;
	};
}
