#include "pch.h"
#include "SimpleInput.h"

#include "KeyboardInput.h"

namespace TY
{
    Float3 SimpleInput::GetPlayerMovement()
    {
        Float3 v{};
        v.x = (KeyD.pressed() ? 1.0f : 0.0f) - (KeyA.pressed() ? 1.0f : 0.0f);
        v.y = (KeyE.pressed() ? 1.0f : 0.0f) - (KeyX.pressed() ? 1.0f : 0.0f);
        v.z = (KeyW.pressed() ? 1.0f : 0.0f) - (KeyS.pressed() ? 1.0f : 0.0f);
        return v;
    }

    Float2 SimpleInput::GetCameraRotation()
    {
        Float2 v{};
        v.x = (KeyRight.pressed() ? 1.0f : 0.0f) - (KeyLeft.pressed() ? 1.0f : 0.0f);
        v.y = (KeyUp.pressed() ? 1.0f : 0.0f) - (KeyDown.pressed() ? 1.0f : 0.0f);
        return v;
    }
}
