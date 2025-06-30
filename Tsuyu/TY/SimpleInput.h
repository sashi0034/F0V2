#pragma once
#include "Vector2D.h"
#include "Vector3D.h"

namespace TY
{
    namespace SimpleInput
    {
        Float2 GetPlayerMovement2D();

        Float3 GetPlayerMovement3D();

        Float2 GetCameraRotation();
    }
}
