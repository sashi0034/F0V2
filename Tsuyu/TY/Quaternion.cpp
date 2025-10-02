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

    float Quaternion::x() const
    {
        return value.m128_f32[0];
    }

    float Quaternion::y() const
    {
        return value.m128_f32[1];
    }

    float Quaternion::z() const
    {
        return value.m128_f32[2];
    }

    float Quaternion::w() const
    {
        return value.m128_f32[3];
    }

    Quaternion Quaternion::Identity()
    {
        return Quaternion{DirectX::XMQuaternionIdentity()};
    }

    Quaternion Quaternion::FromEuler(const Float3& euler)
    {
        using namespace DirectX;
        return XMQuaternionRotationRollPitchYaw(euler.y, euler.x, euler.z);
    }

    Quaternion Quaternion::FromUnitVectors(const Float3& from, const Float3& to)
    {
        using namespace DirectX;

        XMVECTOR vFrom = XMVector3Normalize(XMVECTOR{from.x, from.y, from.z, 0.0f});
        XMVECTOR vTo = XMVector3Normalize(XMVECTOR{to.x, to.y, to.z, 0.0f});

        float cosTheta = XMVectorGetX(XMVector3Dot(vFrom, vTo));
        cosTheta = std::clamp(cosTheta, -1.0f, 1.0f);

        if (cosTheta >= 1.0f - 1e-6f)
        {
            return Quaternion::Identity(); // 同じ方向
        }
        else if (cosTheta <= -1.0f + 1e-6f)
        {
            XMVECTOR orthogonal = XMVector3Normalize(XMVector3Orthogonal(vFrom));
            return XMQuaternionRotationNormal(orthogonal, XM_PI);
        }
        else
        {
            XMVECTOR rotationAxis = XMVector3Normalize(XMVector3Cross(vFrom, vTo));
            float angle = std::acos(cosTheta);
            return XMQuaternionRotationNormal(rotationAxis, angle);
        }
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

    Quaternion Quaternion::inverse() const
    {
        return DirectX::XMQuaternionInverse(value);
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

    Float3 Quaternion::rotate(const Float3& v) const
    {
        using namespace DirectX;

        // 念のため q を正規化（FromVectorsの戻りは単位だけど保険）
        XMVECTOR qn = XMQuaternionNormalize(value);

        XMVECTOR vv = XMVectorSet(v.x, v.y, v.z, 0.0f);
        XMVECTOR rr = XMVector3Rotate(vv, qn); // ← これが最も安全確実

        return Float3(XMVectorGetX(rr), XMVectorGetY(rr), XMVectorGetZ(rr));

        // // 純虚四元数 (vx, vy, vz, w=0)
        // const Quaternion p(v.x, v.y, v.z, 0.0f);
        //
        // // q * p * q^{-1}
        // const Quaternion inv = this->inverse();
        // const Quaternion r = (*this) * p * inv;
        // return Float3(r.x(), r.y(), r.z());
    }

    Quaternion Quaternion::slerp(const Quaternion& q, float t) const
    {
        return DirectX::XMQuaternionSlerp(value, q.value, t);
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
