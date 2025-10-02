#include "pch.h"
#include "Mat4x4.h"

namespace TY
{
    Mat4x4::Mat4x4(Quaternion q)
        : mat{DirectX::XMMatrixRotationQuaternion(q.value)}
    {
    }

    Mat4x4 Mat4x4::rotated(Quaternion quaternion) const
    {
        using namespace DirectX;
        return XMMatrixMultiply(mat, XMMatrixRotationQuaternion(quaternion.value));
    }

    Mat4x4 Mat4x4::transposed() const
    {
        using namespace DirectX;
        return Mat4x4{XMMatrixTranspose(mat)};
    }

    Mat4x4 Mat4x4::inverse() const
    {
        using namespace DirectX;
        return Mat4x4{XMMatrixInverse(nullptr, mat)};
    }

    float Mat4x4::determinant() const
    {
        using namespace DirectX;
        return XMVectorGetX(XMMatrixDeterminant(mat));
    }

    Float3 Mat4x4::eulerRotation() const
    {
        using namespace DirectX;

        XMVECTOR scale;
        XMVECTOR rotQuat;
        XMVECTOR trans;

        if (!XMMatrixDecompose(&scale, &rotQuat, &trans, mat))
        {
            return Vector3D<float>{0.0f, 0.0f, 0.0f};
        }

        const XMMATRIX rotMat = XMMatrixRotationQuaternion(rotQuat);

        const float pitch = std::asin(-rotMat.r[2].m128_f32[1]);

        float yaw, roll;
        if (std::cos(pitch) > 1e-5f)
        {
            yaw = std::atan2(rotMat.r[2].m128_f32[0], rotMat.r[2].m128_f32[2]);
            roll = std::atan2(rotMat.r[0].m128_f32[1], rotMat.r[1].m128_f32[1]);
        }
        else
        {
            yaw = std::atan2(-rotMat.r[0].m128_f32[2], rotMat.r[0].m128_f32[0]);
            roll = 0.0f;
        }

        return Vector3D<float>{pitch, yaw, roll};
    }

    float& Mat4x4::at1(int row, int col)
    {
        assert(1 <= row && row <= 4 && 1 <= col && col <= 4);
        return mat.r[row - 1].m128_f32[col - 1];
    }

    float Mat4x4::at1(int row, int col) const
    {
        assert(1 <= row && row <= 4 && 1 <= col && col <= 4);
        return mat.r[row - 1].m128_f32[col - 1];
    }

    Float3 Mat4x4::transformPoint(const Float3 pos) const noexcept
    {
        using namespace DirectX;
        const XMVECTOR point = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
        const XMVECTOR transformed = XMVector3TransformCoord(point, mat);
        return Float3{
            XMVectorGetX(transformed),
            XMVectorGetY(transformed),
            XMVectorGetZ(transformed)
        };
    }

    Mat4x4 Mat4x4::Translate(const Float3& v) noexcept
    {
        return DirectX::XMMatrixTranslation(v.x, v.y, v.z);
    }

    Mat4x4 Mat4x4::Scale(const Float3& v) noexcept
    {
        return DirectX::XMMatrixScaling(v.x, v.y, v.z);
    }

    Mat4x4 Mat4x4::Rotate(Quaternion quaternion) noexcept
    {
        return DirectX::XMMatrixRotationQuaternion(quaternion.value);
    }

    Mat4x4 Mat4x4::RollPitchYaw(Float3 angles)
    {
        return DirectX::XMMatrixRotationRollPitchYaw(angles.x, angles.y, angles.z);
    }

    Mat4x4 Mat4x4::RollPitchYaw(float roll, float pitch, float yaw)
    {
        return DirectX::XMMatrixRotationRollPitchYaw(roll, pitch, yaw);
    }
}
