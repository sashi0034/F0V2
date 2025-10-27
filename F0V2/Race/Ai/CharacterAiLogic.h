#pragma once
#include "Race/Machine/MachinePhysicsUnit.h"

namespace Race
{
    struct CharacterAiLogicState
    {
        int m_targetWaypointIndex{};
    };

    MachinePhysicsProps::input_t UpdateCharacterAiLogic(
        CharacterAiLogicState& state,
        const MachinePhysicsUnit& machine);
}
