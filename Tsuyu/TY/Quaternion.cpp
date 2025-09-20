#include "pch.h"
#include "Quaternion.h"

namespace TY
{
    Quaternion::Quaternion(float x, float y, float z, float w)
        : value{DirectX::XMVectorSet(x, y, z, w)}
    {
    }

    Quaternion::Quaternion(const Float3& axis, float angle)
        : value{
            DirectX::XMQuaternionRotationNormal(
                DirectX::XMVECTOR{axis.x, axis.y, axis.z, 0.0f}, static_cast<float>(angle))
        }
    {
    }

    Quaternion Quaternion::FromEuler(const Float3& euler)
    {
        using namespace DirectX;
        return XMQuaternionRotationRollPitchYaw(euler.y, euler.x, euler.z);
    }

    Quaternion Quaternion::operator*(const Quaternion& q) const
    {
        return DirectX::XMQuaternionMultiply(value, q.value);
    }

    Quaternion Quaternion::operator*=(const Quaternion& q)
    {
        value = DirectX::XMQuaternionMultiply(value, q.value);
        return *this;
    }

    Float3 Quaternion::eulerAngles() const
    {
        using namespace DirectX;

        float x = XMVectorGetX(value);
        float y = XMVectorGetY(value);
        float z = XMVectorGetZ(value);
        float w = XMVectorGetW(value);

        Float3 angles;

        float sinr_cosp = 2.0f * (w * x + y * z);
        float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
        angles.x = std::atan2(sinr_cosp, cosr_cosp); // Roll

        float sinp = 2.0f * (w * y - z * x);
        if (std::abs(sinp) >= 1.0f)
            angles.y = std::copysign(XM_PI / 2.0f, sinp); // Pitch
        else
            angles.y = std::asin(sinp);

        float siny_cosp = 2.0f * (w * z + x * y);
        float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
        angles.z = std::atan2(siny_cosp, cosy_cosp); // Yaw

        return angles;
    }

    Quaternion Quaternion::RotateX(float angle) noexcept
    {
        using namespace DirectX;
        return XMQuaternionRotationNormal(g_XMIdentityR0, angle);
    }

    Quaternion Quaternion::RotateY(float angle) noexcept
    {
        using namespace DirectX;
        return XMQuaternionRotationNormal(g_XMIdentityR1, angle);
    }

    Quaternion Quaternion::RotateZ(float angle) noexcept
    {
        using namespace DirectX;
        return XMQuaternionRotationNormal(g_XMIdentityR2, angle);
    }
}
