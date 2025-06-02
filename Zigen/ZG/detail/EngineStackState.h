#pragma once
#include <d3d12.h>

#include "ZG/Mat4x4.h"

namespace ZG::detail
{
    // TODO: Obsolete?
    class EngineStackState_impl
    {
    public:
        // void PushPipelineState(const ComPtr<ID3D12PipelineState>& pipelineState) const;
        // void PopPipelineState() const;

        // void PushRootSignature(const ComPtr<ID3D12RootSignature>& rootSignature) const;
        // void PopRootSignature() const;

        void PushWorldMatrix(const Mat4x4& worldMatrix) const;
        void PopWorldMatrix() const;
        [[nodiscard]] Mat4x4 GetWorldMatrix() const;

        void SetViewMatrix(const Mat4x4& viewMatrix) const;
        [[nodiscard]] Mat4x4 GetViewMatrix() const;

        void SetProjectionMatrix(const Mat4x4& projectionMatrix) const;
        [[nodiscard]] Mat4x4 GetProjectionMatrix() const;
    };

    inline constexpr auto EngineStackState = EngineStackState_impl{};
}
