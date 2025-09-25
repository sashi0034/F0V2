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

        float x() const;

        float y() const;

        float z() const;

        float w() const;

        [[nodiscard]]
        static Quaternion Identity();

        [[nodiscard]]
        static Quaternion FromEuler(const Float3& euler);

        [[nodiscard]]
        static Quaternion FromVectors(const Float3& from, const Float3& to);

        [[nodiscard]]
        Quaternion operator*(const Quaternion& q) const;

        Quaternion operator*=(const Quaternion& q);

        [[nodiscard]]
        Quaternion inverse() const;

        [[nodiscard]]
        Float3 eulerAngles() const;

        [[nodiscard]]
        Float3 rotate(const Float3& v) const;

        [[nodiscard]]
        static Quaternion __vectorcall RotateX(float angle) noexcept;

        [[nodiscard]]
        static Quaternion __vectorcall RotateY(float angle) noexcept;

        [[nodiscard]]
        static Quaternion __vectorcall RotateZ(float angle) noexcept;
    };
}
