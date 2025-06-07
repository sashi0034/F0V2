#include "pch.h"
#include "Transformer3D.h"

#include "detail/EngineStateContext.h"

namespace TY
{
    using namespace detail;

    Transformer3D::Transformer3D(const Mat4x4& localWorldMat) : m_active(true)
    {
        EngineStateContext::PushWorldMatrix(localWorldMat);
    }

    Transformer3D::~Transformer3D()
    {
        if (m_active)
        {
            EngineStateContext::PopWorldMatrix();
        }
    }

    void Transformer3D::unsafe_delete()
    {
        delete this;
    }
}
