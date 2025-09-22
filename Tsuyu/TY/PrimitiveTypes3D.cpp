#include "pch.h"
#include "PrimitiveTypes3D.h"

namespace TY
{
    Capsule Capsule::AlongY(const Float3& center, float height, float radius)
    {
        const auto p1 = center + Float3(0, -height * 0.5, 0);
        const auto p2 = center + Float3(0, height * 0.5, 0);
        return Capsule{p1, p2, radius};
    }
}
