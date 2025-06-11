#include "pch.h"
#include "SimpleCamera3D.h"

#include "Graphics3D.h"
#include "Math.h"
#include "SimpleInput.h"
#include "Value3D.h"

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

    Float3 TargetPosition() const
    {
        return Float3{m_eyePosition.x + std::sin(m_yaw), m_targetY, m_eyePosition.z + std::cos(m_yaw)};
    }

    void ApplyMatrix()
    {
        m_cameraMatrix = Mat4x4::LookAt(m_eyePosition, TargetPosition(), m_upDirection);
    }

    void Update(float dt)
    {
        const Float3 moveVector = SimpleInput::GetPlayerMovement();
        const Float2 rotateVector = SimpleInput::GetCameraRotation();

        constexpr double moveSpeed = 10.0f;
        constexpr double rotationSpeed = 50.0f;

        if (not moveVector.isZero())
        {
            const auto forward = m_cameraMatrix.forward();
            const auto df = forward * moveVector.z * moveSpeed * dt;
            m_eyePosition += df;

            const auto right = m_cameraMatrix.right();
            const auto dr = right * moveVector.x * moveSpeed * dt;
            m_eyePosition += dr;

            const auto up = m_cameraMatrix.up();
            const auto du = up * moveVector.y * moveSpeed * dt;
            m_eyePosition += du;

            m_targetY += (df + dr + du).y; // Adjust targetY based on movement
        }

        if (not rotateVector.isZero())
        {
            m_yaw += Math::ToRadians(rotateVector.x * rotationSpeed * dt);

            const auto dy = Abs(m_targetY - m_eyePosition.y);
            const auto s = std::sqrt(1 + dy * dy);
            m_targetY += s * Math::ToRadians(rotateVector.y * rotationSpeed * dt);
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

    void SimpleCamera3D::reset(Float3 eyePosition, Float3 targetPosition, Float3 upDirection)
    {
        *p_impl = {};
        p_impl->m_eyePosition = eyePosition;
        p_impl->m_targetY = targetPosition.y;
        p_impl->m_yaw = std::atan2(targetPosition.x - eyePosition.x, targetPosition.z - eyePosition.z);
        p_impl->m_upDirection = upDirection.normalized();
        p_impl->ApplyMatrix();
    }

    void SimpleCamera3D::update(float dt)
    {
        p_impl->Update(dt);
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
