#pragma once
#include "Vector3D.h"

namespace TY
{
    struct Line3D
    {
        Float3 p0;
        Float3 p1;
    };

    struct Capsule
    {
        Float3 p0;
        Float3 p1;
        float radius;

        static Capsule AlongY(const Float3& center, float height, float radius);
    };
}
