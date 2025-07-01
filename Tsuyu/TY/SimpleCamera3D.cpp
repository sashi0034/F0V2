#include "pch.h"
#include "SimpleCamera3D.h"

#include "Graphics3D.h"
#include "Math.h"
#include "SimpleInput.h"
#include "Vector3D.h"

using namespace TY;

namespace
{
}

struct SimpleCamera3D::Impl
{
    Mat4x4 m_cameraMatrix{};

    Float3 m_eyePosition{};
    double m_yaw{};
    double m_targetY{};
    Float3 m_upDirection{0, 1, 0};

    void SetTargetPosition(Float3 targetPosition)
    {
        m_targetY = targetPosition.y;
        m_yaw = std::atan2(targetPosition.x - m_eyePosition.x, targetPosition.z - m_eyePosition.z);
    }

    Float3 TargetPosition() const
    {
        return Float3{m_eyePosition.x + std::sin(m_yaw), m_targetY, m_eyePosition.z + std::cos(m_yaw)};
    }

    void ApplyMatrix()
    {
        m_cameraMatrix = Mat4x4::LookAt(m_eyePosition, TargetPosition(), m_upDirection);
    }

    void TransformAndApply(float dt, const Float3& moveVector, const Float2& rotateVector)
    {
        if (not moveVector.isZero())
        {
            const auto t = m_cameraMatrix.transposed();

            const auto forward = t.forward();
            const auto df = forward * moveVector.z * dt;
            m_eyePosition += df;

            const auto right = t.right();
            const auto dr = right * moveVector.x * dt;
            m_eyePosition += dr;

            const auto up = t.up();
            const auto du = up * moveVector.y * dt;
            m_eyePosition += du;

            m_targetY += (df + dr + du).y; // Adjust targetY based on movement
        }

        if (not rotateVector.isZero())
        {
            m_yaw += Math::ToRadians(rotateVector.x * dt);

            const auto dy = Abs(m_targetY - m_eyePosition.y);
            const auto s = std::sqrt(1 + dy * dy);
            m_targetY += s * Math::ToRadians(rotateVector.y * dt);
        }

        ApplyMatrix();
    }
};

namespace TY
{
    SimpleCamera3D::SimpleCamera3D() :
        p_impl(std::make_shared<Impl>())
    {
        p_impl->ApplyMatrix();
    }

    void SimpleCamera3D::reset()
    {
        reset(Float3{});
    }

    void SimpleCamera3D::reset(const Float3& eyePosition, const Float3& targetPosition, Float3 upDirection)
    {
        *p_impl = {};

        p_impl->m_eyePosition = eyePosition;

        p_impl->SetTargetPosition(targetPosition);

        p_impl->m_upDirection = upDirection.normalized();

        p_impl->ApplyMatrix();
    }

    void SimpleCamera3D::setEyeAndTarget(const Float3& eyePosition, const Float3& targetPosition)
    {
        p_impl->m_eyePosition = eyePosition;

        p_impl->SetTargetPosition(targetPosition);

        p_impl->ApplyMatrix();
    }

    void SimpleCamera3D::transform(float dt, const Float3& moveVector, const Float2& rotateVector)
    {
        p_impl->TransformAndApply(dt, moveVector, rotateVector);
    }

    void SimpleCamera3D::transformBySimpleInput(float dt, float moveSpeed, float rotateSpeed)
    {
        transform(
            dt,
            SimpleInput::GetPlayerMovement3D() * moveSpeed,
            SimpleInput::GetCameraRotation() * rotateSpeed);
    }

    Float3 SimpleCamera3D::eyePosition() const
    {
        return p_impl->m_eyePosition;
    }

    Float3 SimpleCamera3D::targetPosition() const
    {
        return p_impl->TargetPosition();
    }

    const Mat4x4& SimpleCamera3D::matrix() const
    {
        return p_impl->m_cameraMatrix;
    }
}
