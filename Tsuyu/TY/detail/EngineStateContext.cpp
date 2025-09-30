#include "pch.h"
#include "EngineStateContext.h"

#include <assert.h>

#include "TY/Array.h"
#include "TY/Scene.h"

using namespace TY;

struct EngineStateContextImpl
{
    std::vector<Mat4x4> m_worldMatStack{};
    Mat4x4 m_viewMat{};
    Mat4x4 m_projectionMat{};
    Array<std::unique_ptr<IInlineComponent>> m_components{};
};

namespace
{
    EngineStateContextImpl s_stateContext;
}

namespace TY::detail
{
    void EngineStateContext::Shutdown()
    {
        s_stateContext = {};
    }

    void EngineStateContext::PushWorldMatrix(const Mat4x4& worldMatrix)
    {
        if (s_stateContext.m_worldMatStack.empty())
        {
            s_stateContext.m_worldMatStack.push_back(worldMatrix);
        }
        else
        {
            s_stateContext.m_worldMatStack.push_back(s_stateContext.m_worldMatStack.back() * worldMatrix);
        }
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

    Mat4x4 EngineStateContext::ApplyWorldMatrix(const Mat4x4& matrix)
    {
        if (s_stateContext.m_worldMatStack.empty())
        {
            return matrix;
        }
        else
        {
            return s_stateContext.m_worldMatStack.back() * matrix;
        }
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

    Mat4x4 EngineStateContext::WorldToProjection()
    {
        // TODO: キャッシュ
        return GetWorldMatrix() * s_stateContext.m_viewMat * s_stateContext.m_projectionMat;
    }

    Mat4x4 EngineStateContext::WorldToScreen()
    {
        Mat4x4 m{};
        const float width = static_cast<float>(Scene::Size().x);
        const float height = static_cast<float>(Scene::Size().y);
        m.at1(1, 1) = width / 2.0f;
        m.at1(2, 2) = -height / 2.0f;
        m.at1(3, 3) = 1.0f;
        m.at1(4, 4) = 1.0f;
        m.at1(4, 1) = width / 2.0f;
        m.at1(4, 2) = height / 2.0f;
        return GetWorldMatrix() * s_stateContext.m_viewMat * s_stateContext.m_projectionMat * m;
    }

    IInlineComponent& EngineStateContext::FetchInlineComponent(
        InlineComponentId id,
        const std::function<std::unique_ptr<IInlineComponent>()>& initializer)
    {
        if (s_stateContext.m_components.size() <= id.value())
        {
            s_stateContext.m_components.resize(id.value() + 1);
        }

        if (s_stateContext.m_components[id.value()] == nullptr)
        {
            s_stateContext.m_components[id.value()] = initializer();
        }

        return *s_stateContext.m_components[id.value()];
    }
}
