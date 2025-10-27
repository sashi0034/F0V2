#pragma once
#include "Race/Machine/MachinePhysicsUnit.h"

namespace Race
{
    struct CharacterAiLogicState
    {
        MachinePhysicsProps::input_t input;
    };

    MachinePhysicsProps::input_t UpdateCharacterAiLogic(CharacterAiLogicState& state, const MachinePhysicsUnit& machine);
}
