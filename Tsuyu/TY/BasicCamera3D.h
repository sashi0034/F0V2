#pragma once
#include "Mat4x4.h"
#include "Vector3D.h"

namespace TY
{
    class BasicCamera3D
    {
    public:
        void set(const Float3& eyePosition, const Float3& targetPosition, const Float3& upDirection);

        Float3 eyePosition() const
        {
            return m_eyePosition;
        }

        Float3 targetPosition() const
        {
            return m_targetPosition;
        }

        Float3 upDirection() const
        {
            return m_upDirection;
        }

        const Mat4x4& viewMatrix() const
        {
            return m_viewMatrix;
        }

        const Mat4x4& worldMatrix() const
        {
            return m_worldMatrix;
        }

    private:
        Float3 m_eyePosition{};
        Float3 m_targetPosition{};
        Float3 m_upDirection{0.0f, 1.0f, 0.0f};

        Mat4x4 m_viewMatrix{};
        Mat4x4 m_worldMatrix{};
    };
}
