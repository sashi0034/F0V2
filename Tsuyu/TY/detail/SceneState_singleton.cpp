#include "pch.h"
#include "SceneState_singleton.h"

#include "TY/Mat4x4.h"
#include "TY/Scene.h"

using namespace TY;

struct SceneStateImpl
{
    std::vector<Mat4x4> m_worldMatStack{};
    Mat4x4 m_viewMat{};
    Mat4x4 m_projectionMat{};
};

namespace
{
    SceneStateImpl s_sceneState{};
}

namespace TY
{
    void SceneState_singleton::PushWorldMatrix(const Mat4x4& worldMatrix)
    {
        if (s_sceneState.m_worldMatStack.empty())
        {
            s_sceneState.m_worldMatStack.push_back(worldMatrix);
        }
        else
        {
            s_sceneState.m_worldMatStack.push_back(s_sceneState.m_worldMatStack.back() * worldMatrix);
        }
    }

    void SceneState_singleton::PopWorldMatrix()
    {
        assert(not s_sceneState.m_worldMatStack.empty());
        s_sceneState.m_worldMatStack.pop_back();
    }

    [[nodiscard]] Mat4x4 SceneState_singleton::GetWorldMatrix()
    {
        return s_sceneState.m_worldMatStack.empty() ? Mat4x4::Identity() : s_sceneState.m_worldMatStack.back();
    }

    Mat4x4 SceneState_singleton::ApplyWorldMatrix(const Mat4x4& matrix)
    {
        if (s_sceneState.m_worldMatStack.empty())
        {
            return matrix;
        }
        else
        {
            return s_sceneState.m_worldMatStack.back() * matrix;
        }
    }

    void SceneState_singleton::SetViewMatrix(const Mat4x4& viewMatrix)
    {
        s_sceneState.m_viewMat = viewMatrix;
    }

    Mat4x4 SceneState_singleton::GetViewMatrix()
    {
        return s_sceneState.m_viewMat;
    }

    void SceneState_singleton::SetProjectionMatrix(const Mat4x4& projectionMatrix)
    {
        s_sceneState.m_projectionMat = projectionMatrix;
    }

    Mat4x4 SceneState_singleton::GetProjectionMatrix()
    {
        return s_sceneState.m_projectionMat;
    }

    Mat4x4 SceneState_singleton::WorldToProjection()
    {
        // TODO: キャッシュ
        return GetWorldMatrix() * s_sceneState.m_viewMat * s_sceneState.m_projectionMat;
    }

    Mat4x4 SceneState_singleton::WorldToScreen()
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
        return GetWorldMatrix() * s_sceneState.m_viewMat * s_sceneState.m_projectionMat * m;
    }
}
