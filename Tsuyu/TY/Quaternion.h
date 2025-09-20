#pragma once
#include "Vector3D.h"

namespace TY
{
    struct alignas(16) Quaternion
    {
        DirectX::XMVECTOR value{0.0f, 0.0f, 0.0f, 1.0f};

        [[nodiscard]]
        Quaternion() = default;

        [[nodiscard]]
        Quaternion(const DirectX::XMVECTOR& v)
            : value(v)
        {
        }

        [[nodiscard]]
        Quaternion(float x, float y, float z, float w);

        [[nodiscard]]
        Quaternion(const Float3& axis, float angle);

        [[nodiscard]]
        static Quaternion FromEuler(const Float3& euler);

        [[nodiscard]]
        Quaternion operator*(const Quaternion& q) const;

        Quaternion operator*=(const Quaternion& q);

        [[nodiscard]]
        Float3 eulerAngles() const;

        [[nodiscard]]
        static Quaternion __vectorcall RotateX(float angle) noexcept;

        [[nodiscard]]
        static Quaternion __vectorcall RotateY(float angle) noexcept;

        [[nodiscard]]
        static Quaternion __vectorcall RotateZ(float angle) noexcept;
    };
}
