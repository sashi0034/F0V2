#include "pch.h"
#include "Pose.h"

namespace TY
{
    Mat4x4 Pose::getMatrix() const
    {
        return Mat4x4::Rotate(rotation).translated(position);
    }

    Float3 Pose::eulerAngles() const
    {
        return rotation.eulerAngles();
    }
}
