#include "pch.h"
#include "Mat4x4.h"

namespace TY
{
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

    Mat4x4 Mat4x4::RollPitchYaw(Float3 angles)
    {
        return DirectX::XMMatrixRotationRollPitchYaw(angles.x, angles.y, angles.z);
    }

    Mat4x4 Mat4x4::RollPitchYaw(float roll, float pitch, float yaw)
    {
        return DirectX::XMMatrixRotationRollPitchYaw(roll, pitch, yaw);
    }
}
