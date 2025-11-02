#include "pch.h"
#include "SceneState3D_singleton.h"

#include "TY/Mat4x4.h"
#include "TY/Screen.h"

using namespace TY;

struct SceneStateImpl
{
    bool m_shouldRefresh{};

    Array<Mat4x4> m_worldMatStack{};

    Mat4x4 m_viewMat{};

    // TODO: worldToView には m_worldMatStack が適応されるようにする

    Mat4x4 m_projectionMat{};
};

namespace
{
    SceneStateImpl s_sceneState{};
}

namespace TY
{
    bool SceneState3D_singleton::ShouldRefresh()
    {
        return s_sceneState.m_shouldRefresh;
    }

    void SceneState3D_singleton::OnRefreshed()
    {
        s_sceneState.m_shouldRefresh = false;
    }

    void SceneState3D_singleton::PushWorldMatrix(const Mat4x4& worldMatrix)
    {
        s_sceneState.m_shouldRefresh = true;

        if (s_sceneState.m_worldMatStack.empty())
        {
            s_sceneState.m_worldMatStack.push_back(worldMatrix);
        }
        else
        {
            s_sceneState.m_worldMatStack.push_back(s_sceneState.m_worldMatStack.back() * worldMatrix);
        }
    }

    void SceneState3D_singleton::PopWorldMatrix()
    {
        s_sceneState.m_shouldRefresh = true;

        assert(not s_sceneState.m_worldMatStack.empty());
        s_sceneState.m_worldMatStack.pop_back();
    }

    [[nodiscard]] Mat4x4 SceneState3D_singleton::GetWorldMatrix()
    {
        return s_sceneState.m_worldMatStack.empty() ? Mat4x4::Identity() : s_sceneState.m_worldMatStack.back();
    }

    Mat4x4 SceneState3D_singleton::ApplyWorldMatrix(const Mat4x4& matrix)
    {
        s_sceneState.m_shouldRefresh = true;

        if (s_sceneState.m_worldMatStack.empty())
        {
            return matrix;
        }
        else
        {
            return s_sceneState.m_worldMatStack.back() * matrix;
        }
    }

    void SceneState3D_singleton::SetViewMatrix(const Mat4x4& viewMatrix)
    {
        s_sceneState.m_shouldRefresh = true;

        s_sceneState.m_viewMat = viewMatrix;
    }

    Mat4x4 SceneState3D_singleton::GetViewMatrix()
    {
        return s_sceneState.m_viewMat;
    }

    void SceneState3D_singleton::SetProjectionMatrix(const Mat4x4& projectionMatrix)
    {
        s_sceneState.m_shouldRefresh = true;

        s_sceneState.m_projectionMat = projectionMatrix;
    }

    Mat4x4 SceneState3D_singleton::GetProjectionMatrix()
    {
        return s_sceneState.m_projectionMat;
    }

    Mat4x4 SceneState3D_singleton::WorldToProjection()
    {
        // TODO: キャッシュ
        return GetWorldMatrix() * s_sceneState.m_viewMat * s_sceneState.m_projectionMat;
    }

    Mat4x4 SceneState3D_singleton::WorldToScreen()
    {
        Mat4x4 m{};
        const float width = static_cast<float>(Screen::Size().x);
        const float height = static_cast<float>(Screen::Size().y);
        m.at1(1, 1) = width / 2.0f;
        m.at1(2, 2) = -height / 2.0f;
        m.at1(3, 3) = 1.0f;
        m.at1(4, 4) = 1.0f;
        m.at1(4, 1) = width / 2.0f;
        m.at1(4, 2) = height / 2.0f;
        return GetWorldMatrix() * s_sceneState.m_viewMat * s_sceneState.m_projectionMat * m;
    }
}
