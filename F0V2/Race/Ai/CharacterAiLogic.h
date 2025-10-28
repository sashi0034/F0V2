#pragma once
#include "Race/Machine/MachinePhysicsUnit.h"

namespace Race
{
    struct CharacterAiLogicState
    {
        int m_targetWaypointIndex{};
        int m_lastLapIndex{};
        float m_accumulatedRightHandling{};
    };

    MachinePhysicsProps::input_t UpdateCharacterAiLogic(
        CharacterAiLogicState& state,
        const MachinePhysicsUnit& machine);
}
