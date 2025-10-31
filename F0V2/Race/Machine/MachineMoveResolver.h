#pragma once
#include "MachinePhysics.h"

namespace Race
{
    void ResolveMachineMove(
        MachinePhysicsState& state,
        const Float3& moveVector,
        const MachinePhysicsProps& props);

    void ResolveMachineGroundContact(MachinePhysicsState& state);
}
