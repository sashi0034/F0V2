#include "pch.h"
#include "Transformer3D.h"

#include "detail/EngineStackState.h"

namespace ZG
{
    using namespace detail;

    Transformer3D::Transformer3D(const Mat4x4& localWorldMat) : m_active(true)
    {
        EngineStackState.PushWorldMatrix(localWorldMat);
    }

    Transformer3D::~Transformer3D()
    {
        if (m_active)
        {
            EngineStackState.PopWorldMatrix();
        }
    }

    void Transformer3D::unsafe_delete()
    {
        delete this;
    }
}
