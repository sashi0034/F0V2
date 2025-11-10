#include "pch.h"
#include "BasicCamera3D.h"

namespace TY
{
    void BasicCamera3D::set(const Float3& eyePosition, const Float3& targetPosition, const Float3& upDirection)
    {
        m_eyePosition = eyePosition;

        m_targetPosition = targetPosition;

        m_upDirection = upDirection;

        m_viewMatrix = Mat4x4::LookAt(
            m_eyePosition,
            m_targetPosition,
            m_upDirection
        );

        m_worldMatrix = m_viewMatrix.transposed();
    }
}
