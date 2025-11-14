#pragma once
#include "CharacterAiInputCommand.h"
#include "Race/Machine/MachinePhysicsUnit.h"

namespace Race
{
    struct CharacterAiLogicState
    {
        CharacterAiInputCommand m_inputCommand;

        int m_aiId{};
        int m_lastLapIndex{};
    };

    MachinePhysicsProps::input_t UpdateCharacterAiLogic(
        CharacterAiLogicState& state,
        const MachinePhysicsUnit& machine);
}
