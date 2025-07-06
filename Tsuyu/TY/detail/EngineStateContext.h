#pragma once

#include "TY/InlineComponent.h"
#include "TY/Mat4x4.h"

namespace TY::detail
{
    namespace EngineStateContext
    {
        void Shutdown();

        void PushWorldMatrix(const Mat4x4& worldMatrix);
        void PopWorldMatrix();
        [[nodiscard]] Mat4x4 GetWorldMatrix();
        [[nodiscard]] Mat4x4 ApplyWorldMatrix(const Mat4x4& matrix);

        void SetViewMatrix(const Mat4x4& viewMatrix);
        [[nodiscard]] Mat4x4 GetViewMatrix();

        void SetProjectionMatrix(const Mat4x4& projectionMatrix);
        [[nodiscard]] Mat4x4 GetProjectionMatrix();

        IInlineComponent& FetchInlineComponent(
            InlineComponentId id,
            const std::function<std::unique_ptr<IInlineComponent>()>& initializer);
    };
}
