#include "pch.h"
#include "Transformer3D.h"

#include "detail/EngineStateContext.h"
#include "detail/SceneState3D_singleton.h"

namespace TY
{
    using namespace detail;

    Transformer3D::Transformer3D(const Mat4x4& localWorldMat) : m_active(true)
    {
        SceneState3D_singleton::PushWorldMatrix(localWorldMat);
    }

    Transformer3D::~Transformer3D()
    {
        if (m_active)
        {
            SceneState3D_singleton::PopWorldMatrix();
        }
    }

    void Transformer3D::unsafe_delete()
    {
        delete this;
    }
}
