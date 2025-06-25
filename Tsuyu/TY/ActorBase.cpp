#include "pch.h"
#include "ActorBase.h"

namespace TY
{
    void ActorBase::kill()
    {
        if (m_alive == false) return;
        m_alive = false;
        killed();
    }
}
