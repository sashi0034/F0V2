#pragma once
#include "TY/Mat4x4.h"

namespace TY
{
    namespace SceneState3D_singleton
    {
        bool ShouldRefresh();

        void OnRefreshed();

        void PushWorldMatrix(const Mat4x4& worldMatrix);

        void PopWorldMatrix();

        [[nodiscard]]
        Mat4x4 GetWorldMatrix();

        [[nodiscard]]
        Mat4x4 ApplyWorldMatrix(const Mat4x4& matrix);

        void SetViewMatrix(const Mat4x4& viewMatrix);

        [[nodiscard]]
        Mat4x4 GetViewMatrix();

        void SetProjectionMatrix(const Mat4x4& projectionMatrix);

        [[nodiscard]]
        Mat4x4 GetProjectionMatrix();

        [[nodiscard]]
        Mat4x4 WorldToProjection();

        [[nodiscard]]
        Mat4x4 WorldToScreen();
    }
}
