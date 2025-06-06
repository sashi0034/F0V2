#pragma once

#include "Value3D.h"

namespace TY
{
    struct alignas(16) Mat4x4
    {
        DirectX::XMMATRIX mat;

        Mat4x4() = default;

        Mat4x4(const DirectX::XMMATRIX& xm) : mat(xm)
        {
        }

        [[nodiscard]] Mat4x4 translated(const Float3& v) const
        {
            return DirectX::XMMatrixMultiply(mat, DirectX::XMMatrixTranslation(v.x, v.y, v.z));
        }

        [[nodiscard]] Mat4x4 translatedX(const Float3& v) const
        {
            return DirectX::XMMatrixMultiply(mat, DirectX::XMMatrixTranslation(v.x, 0.0f, 0.0f));
        }

        [[nodiscard]] Mat4x4 translatedY(const Float3& v) const
        {
            return DirectX::XMMatrixMultiply(mat, DirectX::XMMatrixTranslation(0.0f, v.y, 0.0f));
        }

        [[nodiscard]] Mat4x4 translatedZ(const Float3& v) const
        {
            return DirectX::XMMatrixMultiply(mat, DirectX::XMMatrixTranslation(0.0f, 0.0f, v.z));
        }

        [[nodiscard]] Mat4x4 translated(float x, float y, float z) const
        {
            return DirectX::XMMatrixMultiply(
                mat, DirectX::XMMatrixTranslation(x, y, z));
        }

        [[nodiscard]] Mat4x4 scaled(const Float3& v) const
        {
            return DirectX::XMMatrixMultiply(
                mat, DirectX::XMMatrixScaling(v.x, v.y, v.z));
        }

        [[nodiscard]] Mat4x4 scaled(float x, float y, float z) const
        {
            return DirectX::XMMatrixMultiply(
                mat, DirectX::XMMatrixScaling(x, y, z));
        }

        [[nodiscard]] Mat4x4 rotatedX(float angle) const
        {
            return Mat4x4{DirectX::XMMatrixRotationX(angle) * mat};
        }

        [[nodiscard]] Mat4x4 rotatedY(float angle) const
        {
            return Mat4x4{DirectX::XMMatrixRotationY(angle) * mat};
        }

        [[nodiscard]] Mat4x4 rotatedZ(float angle) const
        {
            return Mat4x4{DirectX::XMMatrixRotationZ(angle) * mat};
        }

        [[nodiscard]] Float3 translation() const
        {
            using namespace DirectX;

            return Vector3D<float>{
                mat.r[3].m128_f32[0],
                mat.r[3].m128_f32[1],
                mat.r[3].m128_f32[2]
            };
        }

        [[nodiscard]] Float3 eulerRotation() const;

        [[nodiscard]] Vector3D<float> up() const
        {
            using namespace DirectX;

            return Vector3D<float>{
                mat.r[0].m128_f32[1],
                mat.r[1].m128_f32[1],
                mat.r[2].m128_f32[1]
            };
        }

        [[nodiscard]] Vector3D<float> right() const
        {
            using namespace DirectX;

            return Vector3D<float>{
                mat.r[0].m128_f32[0],
                mat.r[1].m128_f32[0],
                mat.r[2].m128_f32[0]
            };
        }

        [[nodiscard]] Vector3D<float> forward() const
        {
            using namespace DirectX;

            return Vector3D<float>{
                mat.r[0].m128_f32[2],
                mat.r[1].m128_f32[2],
                mat.r[2].m128_f32[2]
            };
        }

        [[nodiscard]] Mat4x4 operator*(const Mat4x4& rhs) const
        {
            return Mat4x4{mat * rhs.mat};
        }

        template <typename T>
        [[nodiscard]] Vector3D<T> operator*(const Vector3D<T>& rhs) const
        {
            return Vector3D<T>{XMVector3Transform(rhs.toXMV(), mat)};
        }

        [[nodiscard]] static Mat4x4 Identity()
        {
            return Mat4x4{DirectX::XMMatrixIdentity()};
        }

        /// @brief 透視投影行列を生成する (左手座標系)
        template <typename T>
        [[nodiscard]] static Mat4x4 LookAt(const Vector3D<T>& eye, const Vector3D<T>& target, const Vector3D<T>& up)
        {
            return Mat4x4{DirectX::XMMatrixLookAtLH(eye.toXMV(), target.toXMV(), up.toXMV())};
        }

        [[nodiscard]] static Mat4x4 PerspectiveFov(
            float fov, float aspect, float nearZ, float farZ)
        {
            return Mat4x4{DirectX::XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ)};
        }

        [[nodiscard]] static Mat4x4 Translate(const Float3 v) noexcept
        {
            return DirectX::XMMatrixTranslation(v.x, v.y, v.z);
        }

        [[nodiscard]] static Mat4x4 RollPitchYaw(Float3 angles);

        [[nodiscard]] static Mat4x4 RollPitchYaw(float roll, float pitch, float yaw);
    };
}
