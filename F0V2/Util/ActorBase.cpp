#include "pch.h"
#include "ActorBase.h"

namespace Util
{
	void ActorBase::Kill()
	{
		if (m_alive == false) return;
		m_alive = false;
		Killed();
	}
}
