#pragma once
#include "Vector3D.h"

namespace TY
{
    struct Triangle3D
    {
        Float3 p0;
        Float3 p1;
        Float3 p2;

        [[nodiscard]] Float3 getNormal() const;
    };
}
