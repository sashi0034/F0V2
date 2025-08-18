#pragma once
#include "CommandList.h"
#include "IEngineDrawer.h"
#include "PipelineType.h"
#include "TY/ConstantBuffer.h"
#include "TY/Mat3x2.h"
#include "TY/Mat4x4.h"
#include "TY/RenderTarget.h"
#include "TY/Vector2D.h"

namespace TY::detail
{
    struct SceneState3D_b0
    {
        Mat4x4 projectionMatrix;
        Mat4x4 viewMatrix;
    };

    namespace EngineRenderContext
    {
        constexpr int FrameBufferCount = 2;

        void Init();

        void NewFrame();

        void Render();

        void Shutdown();

        const RenderTarget& GetBackBuffer();

        [[nodiscard]]
        ID3D12Device* GetDevice();

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCommandList(CommandListType type);

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCommandList(PipelineType type);

        void FlushComputeCommandSync();

        void RequestFrameBufferSize(Size frameBufferSize);

        [[nodiscard]]
        Size FrameBufferSize();

        [[nodiscard]]
        Mat3x2 WindowToFrameBuffer();

        [[nodiscard]]
        ConstantBuffer<SceneState3D_b0> GetSceneState3D_CB0();

        void MarkDrawerUntilFlush(const std::shared_ptr<IEngineDrawer>& drawer);

        size_t GetFlushTimestamp();
    }
}
