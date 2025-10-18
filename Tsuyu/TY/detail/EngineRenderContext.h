#pragma once
#include "CommandListManager.h"
#include "PipelineType.h"
#include "TY/ConstantBuffer.h"
#include "TY/IGpuMemoryUsage.h"
#include "TY/Mat3x2.h"
#include "TY/Mat4x4.h"
#include "TY/RenderTarget.h"
#include "TY/Variant.h"

namespace TY::detail
{
    struct SceneState3D_b0
    {
        Mat4x4 projectionMatrix;
        Mat4x4 viewMatrix;
    };

    using RenderResource = Variant<
        ComPtr<ID3DBlob>,
        ComPtr<ID3D12Resource>,
        ComPtr<ID3D12PipelineState>,
        ComPtr<ID3D12RootSignature>,
        ComPtr<ID3D12DescriptorHeap>
    >;

    namespace EngineRenderContext
    {
        constexpr int FrameBufferCount = 2;

        void Init();

        void NewFrame();

        void Render();

        void Shutdown();

        [[nodiscard]]
        ID3D12Device* GetDevice();

        [[nodiscard]]
        ID3D12GraphicsCommandList* TargetCommandList();

        void FlushComputeCommandSync();

        void RequestFrameBufferSize(Size frameBufferSize);

        void RequestFullscreen(bool fullscreen);

        [[nodiscard]]
        bool IsFullscreen();

        [[nodiscard]]
        Size FrameBufferSize();

        [[nodiscard]]
        Mat3x2 WindowToFrameBuffer();

        [[nodiscard]]
        Mat3x2 FrameBufferToWindow();

        [[nodiscard]]
        ConstantBuffer<SceneState3D_b0> GetSceneState3D_CB0();

        void SafeDisposeRenderResource(const RenderResource& renderResource);

        size_t GetFlushTimestamp();

        IGpuMemoryUsage& GpuMemoryUsage();
    }
}
