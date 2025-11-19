#include "pch.h"
#include "SimpleInput.h"

#include "Gamepad.h"
#include "KeyboardInput.h"

namespace TY
{
    Float2 SimpleInput::GetPlayerMovement2D()
    {
        Float2 v{};

        if (IsUsingGamepad())
        {
            const auto axisL = MainGamepad.axisL();
            v.x += axisL.x;
            v.y += axisL.y;
        }
        else
        {
            v.x = (KeyD.pressed() ? 1.0f : 0.0f) - (KeyA.pressed() ? 1.0f : 0.0f);
            v.y = (KeyS.pressed() ? 1.0f : 0.0f) - (KeyW.pressed() ? 1.0f : 0.0f);
        }

        return v;
    }

    Float3 SimpleInput::GetPlayerMovement3D()
    {
        Float3 v{};

        if (IsUsingGamepad())
        {
            const auto axisL = MainGamepad.axisL();;
            v.x += axisL.x;
            v.z += -axisL.y;
            v.y += (MainGamepad.rb().pressed ? 1.0f : 0.0f) - (MainGamepad.lb().pressed ? 1.0f : 0.0f);
        }
        else
        {
            v.x = (KeyD.pressed() ? 1.0f : 0.0f) - (KeyA.pressed() ? 1.0f : 0.0f);
            v.y = (KeyE.pressed() ? 1.0f : 0.0f) - (KeyX.pressed() ? 1.0f : 0.0f);
            v.z = (KeyW.pressed() ? 1.0f : 0.0f) - (KeyS.pressed() ? 1.0f : 0.0f);
        }

        return v;
    }

    Float2 SimpleInput::GetCameraRotation()
    {
        Float2 v{};

        if (IsUsingGamepad())
        {
            const auto axisR = MainGamepad.axisR();
            v.x += axisR.x;
            v.y += -axisR.y; // FIXME?
        }
        else
        {
            v.x = (KeyRight.pressed() ? 1.0f : 0.0f) - (KeyLeft.pressed() ? 1.0f : 0.0f);
            v.y = (KeyUp.pressed() ? 1.0f : 0.0f) - (KeyDown.pressed() ? 1.0f : 0.0f);
        }

        return v;
    }
}
