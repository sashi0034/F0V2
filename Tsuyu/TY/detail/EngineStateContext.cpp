#include "pch.h"
#include "EngineStateContext.h"

#include <assert.h>

using namespace TY;

struct EngineStateContextImpl
{
    std::vector<Mat4x4> m_worldMatStack{};
    Mat4x4 m_viewMat{};
    Mat4x4 m_projectionMat{};
};

namespace
{
    EngineStateContextImpl s_stateContext;
}

namespace TY::detail
{
    void EngineStateContext::PushWorldMatrix(const Mat4x4& worldMatrix)
    {
        s_stateContext.m_worldMatStack.push_back(worldMatrix);
    }

    void EngineStateContext::PopWorldMatrix()
    {
        assert(not s_stateContext.m_worldMatStack.empty());
        s_stateContext.m_worldMatStack.pop_back();
    }

    [[nodiscard]] Mat4x4 EngineStateContext::GetWorldMatrix()
    {
        return s_stateContext.m_worldMatStack.empty() ? Mat4x4::Identity() : s_stateContext.m_worldMatStack.back();
    }

    void EngineStateContext::SetViewMatrix(const Mat4x4& viewMatrix)
    {
        s_stateContext.m_viewMat = viewMatrix;
    }

    [[nodiscard]] Mat4x4 EngineStateContext::GetViewMatrix()
    {
        return s_stateContext.m_viewMat;
    }

    void EngineStateContext::SetProjectionMatrix(const Mat4x4& projectionMatrix)
    {
        s_stateContext.m_projectionMat = projectionMatrix;
    }

    [[nodiscard]] Mat4x4 EngineStateContext::GetProjectionMatrix()
    {
        return s_stateContext.m_projectionMat;
    }
}
