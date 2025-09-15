#pragma once
#include "TY/Vector3D.h"

namespace TY
{
    struct SerializeTransform
    {
        std::string tag{};
        Float3 position{};
        Float3 rotation{};
        Float3 scale{1.0f, 1.0f, 1.0f};
    };
}
