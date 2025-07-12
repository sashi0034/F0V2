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
    Mat4x4 m_viewMatrix{};
    Mat4x4 m_worldMatrix{};

    Float3 m_eyePosition{};
    double m_yaw{};
    double m_targetY{};
    Float3 m_upDirection{0, 1, 0};

    void SetTargetPosition(const Float3& targetPosition)
    {
        const auto eyeToTarget = targetPosition - m_eyePosition;
        const float h = eyeToTarget.y / std::sqrt(eyeToTarget.x * eyeToTarget.x + eyeToTarget.z * eyeToTarget.z);
        m_targetY = m_eyePosition.y + h;
        m_yaw = std::atan2(eyeToTarget.x, eyeToTarget.z);
    }

    Float3 TargetPosition() const
    {
        return Float3{m_eyePosition.x + std::sin(m_yaw), m_targetY, m_eyePosition.z + std::cos(m_yaw)};
    }

    void ApplyMatrix()
    {
        m_viewMatrix = Mat4x4::LookAt(m_eyePosition, TargetPosition(), m_upDirection);
        m_worldMatrix = m_viewMatrix.transposed();
    }

    void TransformAndApply(float dt, const Float3& moveVector, const Float2& rotateVector)
    {
        if (not moveVector.isZero())
        {
            const auto forward = m_worldMatrix.forward();
            const auto df = forward * moveVector.z * dt;
            m_eyePosition += df;

            const auto right = m_worldMatrix.right();
            const auto dr = right * moveVector.x * dt;
            m_eyePosition += dr;

            const auto up = m_worldMatrix.up();
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

    const Mat4x4& SimpleCamera3D::viewMatrix() const
    {
        return p_impl->m_viewMatrix;
    }

    const Mat4x4& SimpleCamera3D::worldMatrix() const
    {
        return p_impl->m_worldMatrix;
    }
}
