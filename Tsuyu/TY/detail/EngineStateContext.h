#pragma once

#include "TY/Mat4x4.h"

namespace TY::detail
{
    namespace EngineStateContext
    {
        void PushWorldMatrix(const Mat4x4& worldMatrix);
        void PopWorldMatrix();
        [[nodiscard]] Mat4x4 GetWorldMatrix();

        void SetViewMatrix(const Mat4x4& viewMatrix);
        [[nodiscard]] Mat4x4 GetViewMatrix();

        void SetProjectionMatrix(const Mat4x4& projectionMatrix);
        [[nodiscard]] Mat4x4 GetProjectionMatrix();
    };
}
