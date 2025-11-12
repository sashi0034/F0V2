#pragma once
#include "MachinePhysics.h"

namespace Race
{
    struct MachinePhysicsUnit
    {
        MachinePhysicsState state{};
        MachinePhysicsProps props{};

        MachineId id() const
        {
            return props.machineId;
        }
    };
}
