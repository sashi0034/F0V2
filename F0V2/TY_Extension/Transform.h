#pragma once
#include "TY/Mat4x4.h"
#include "TY/Quaternion.h"
#include "TY/Vector3D.h"

namespace TY
{
    struct Transform
    {
        Float3 position;
        Quaternion rotation;
        Float3 scale;

        Mat4x4 getMatrix() const
        {
            return Mat4x4::Identity()
                   .scaled(scale)
                   .rotated(rotation)
                   .translated(position);
        }
    };
}
