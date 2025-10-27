#include "pch.h"
#include "CharacterAiLogic.h"

namespace Race
{
    MachinePhysicsProps::input_t UpdateCharacterAiLogic(CharacterAiLogicState& state, const MachineUnit& machine)
    {
        MachinePhysicsProps::input_t input{};
        if (machine.state.m_velocity.lengthSq() < Math::Square(100.0f))
        {
            input.accelPressed = true;
        }
        else
        {
            input.accelPressed = false;
        }

        input.rightHandling = 1.0f;

        return input;
    }
}
