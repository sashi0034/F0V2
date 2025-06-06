#include "pch.h"
#include "EngineStackState.h"

#include <assert.h>

#include "EngineCore.h"

namespace
{
    using namespace TY;

    struct Impl
    {
        // std::vector<ComPtr<ID3D12PipelineState>> m_pipelineStateStack{};
        // std::vector<ComPtr<ID3D12RootSignature>> m_rootSignatureStack{};

        std::vector<Mat4x4> m_worldMatStack{};
        Mat4x4 m_viewMat{};
        Mat4x4 m_projectionMat{};
    } s_engineStack;
}

namespace TY::detail
{
    // void EngineStackState_impl::PushPipelineState(const ComPtr<ID3D12PipelineState>& pipelineState) const
    // {
    //     assert(pipelineState);
    //     s_impl.m_pipelineStateStack.push_back(pipelineState);
    //
    //     const auto commandList = EngineCore.GetCommandList();
    //     commandList->SetPipelineState(pipelineState.Get());
    // }
    //
    // void EngineStackState_impl::PopPipelineState() const
    // {
    //     assert(not s_impl.m_pipelineStateStack.empty());
    //     s_impl.m_pipelineStateStack.pop_back();
    //
    //     if (not s_impl.m_pipelineStateStack.empty())
    //     {
    //         const auto commandList = EngineCore.GetCommandList();
    //         commandList->SetPipelineState(s_impl.m_pipelineStateStack.back().Get());
    //     }
    // }
    //
    // void EngineStackState_impl::PushRootSignature(const ComPtr<ID3D12RootSignature>& rootSignature) const
    // {
    //     assert(rootSignature);
    //     s_impl.m_rootSignatureStack.push_back(rootSignature);
    //
    //     const auto commandList = EngineCore.GetCommandList();
    //     commandList->SetGraphicsRootSignature(rootSignature.Get());
    // }
    //
    // void EngineStackState_impl::PopRootSignature() const
    // {
    //     assert(not s_impl.m_rootSignatureStack.empty());
    //     s_impl.m_rootSignatureStack.pop_back();
    //
    //     if (not s_impl.m_rootSignatureStack.empty())
    //     {
    //         const auto commandList = EngineCore.GetCommandList();
    //         commandList->SetGraphicsRootSignature(s_impl.m_rootSignatureStack.back().Get());
    //     }
    // }
    void EngineStackState_impl::PushWorldMatrix(const Mat4x4& worldMatrix) const
    {
        s_engineStack.m_worldMatStack.push_back(worldMatrix);
    }

    void EngineStackState_impl::PopWorldMatrix() const
    {
        assert(not s_engineStack.m_worldMatStack.empty());
        s_engineStack.m_worldMatStack.pop_back();
    }

    [[nodiscard]] Mat4x4 EngineStackState_impl::GetWorldMatrix() const
    {
        return s_engineStack.m_worldMatStack.empty() ? Mat4x4::Identity() : s_engineStack.m_worldMatStack.back();
    }

    void EngineStackState_impl::SetViewMatrix(const Mat4x4& viewMatrix) const
    {
        s_engineStack.m_viewMat = viewMatrix;
    }

    [[nodiscard]] Mat4x4 EngineStackState_impl::GetViewMatrix() const
    {
        return s_engineStack.m_viewMat;
    }

    void EngineStackState_impl::SetProjectionMatrix(const Mat4x4& projectionMatrix) const
    {
        s_engineStack.m_projectionMat = projectionMatrix;
    }

    [[nodiscard]] Mat4x4 EngineStackState_impl::GetProjectionMatrix() const
    {
        return s_engineStack.m_projectionMat;
    }
}
