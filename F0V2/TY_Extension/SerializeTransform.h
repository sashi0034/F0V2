#pragma once
#include "TY/Mat4x4.h"
#include "TY/Vector3D.h"

namespace TY
{
    struct SerializeTransform
    {
        Float3 position{};
        Float3 rotation{};
        Float3 scale{1.0f, 1.0f, 1.0f};

        Mat4x4 getMatrix() const
        {
            return Mat4x4::Identity()
                   .rotated(Quaternion::FromEuler(rotation))
                   .scaled(scale)
                   .translated(position);
        }

        static SerializeTransform Deserialize(const toml::table& tbl);

        toml::table serialize() const;
    };
}
