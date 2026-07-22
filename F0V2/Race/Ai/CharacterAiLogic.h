#pragma once
#include "CharacterAIInputCommand.h"
#include "Race/Machine/MachinePhysicsUnit.h"

namespace Race
{
    struct CharacterAILogicState
    {
        CharacterAIInputCommand m_inputCommand;

        int m_aiId{};
        int m_lastLapIndex{};
    };

    MachinePhysicsProps::input_t UpdateCharacterAILogic(
        CharacterAILogicState& state,
        const MachinePhysicsUnit& machine);
}
