#pragma once
#include "TY/Mat4x4.h"
#include "TY/Vector3D.h"

namespace TY
{
    struct Pose
    {
        Float3 position{};
        Quaternion rotation{};

        [[nodiscard]]
        Mat4x4 getMatrix() const;

        [[nodiscard]]
        Float3 eulerAngles() const;
    };
}
