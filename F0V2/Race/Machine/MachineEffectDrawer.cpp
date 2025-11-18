#include "pch.h"
#include "MachineEffectDrawer.h"

#include "Util/ImmediatePrint.h"

using namespace Race;

struct MachineEffectDrawer::Impl
{
};

namespace Race
{
    MachineEffectDrawer::MachineEffectDrawer() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void MachineEffectDrawer::update()
    {
    }

    void MachineEffectDrawer::drawTransparent() const
    {
        ImmediatePrint(U"TODO");
    }
}
