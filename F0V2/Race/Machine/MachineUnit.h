#pragma once
#include "MachinePhysics.h"

namespace Race
{
    struct MachineUnit
    {
        MachinePhysicsState state{};
        MachinePhysicsProps props{};
    };
}
