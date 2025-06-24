#pragma once
#include "CommandList.h"
#include "PipelineType.h"
#include "TY/Mat3x2.h"
#include "TY/RenderTarget.h"
#include "TY/Vector2D.h"

namespace TY::detail
{
    namespace EngineRenderContext
    {
        void Init();

        void NewFrame();

        void Render();

        void Shutdown();

        const RenderTarget& GetBackBuffer();

        [[nodiscard]]
        ID3D12Device* GetDevice();

        [[nodiscard]]
        ScopedDefer ScopedCommandTarget(CommandListType type);

        [[nodiscard]]
        CommandListType ActiveCommandTarget();

        [[nodiscard]]
        ID3D12GraphicsCommandList* ActiveCommandList();

        void FlushActiveCommandList();

        void RequestFrameBufferSize(Size frameBufferSize);

        Size FrameBufferSize();

        Mat3x2 WindowToFrameBuffer();
    }
}
