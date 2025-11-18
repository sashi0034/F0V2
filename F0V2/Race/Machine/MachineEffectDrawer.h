#pragma once
#include "MachinePhysicsUnit.h"

namespace Race
{
    class MachineEffectDrawer
    {
    public:
        MachineEffectDrawer();

        void init();

        void update(const MachinePhysicsUnit& machine);

        void drawTransparent() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
