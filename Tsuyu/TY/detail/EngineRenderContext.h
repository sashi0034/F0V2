#pragma once
#include "CommandList.h"
#include "PipelineType.h"
#include "TY/RenderTarget.h"
#include "TY/Value2D.h"

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

        Size GetSceneSize();
    }
}
