#pragma once
#include "TY/Vector3D.h"

inline namespace CB
{
    struct Lambert_b10
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
    };
}
