#pragma once
#include "Race/Machine/MachinePhysicsUnit.h"

namespace Race
{
    struct CharacterAiLogicState
    {
        int m_aiId{};
        int m_lastLapIndex{};
    };

    MachinePhysicsProps::input_t UpdateCharacterAiLogic(
        CharacterAiLogicState& state,
        const MachinePhysicsUnit& machine);
}
