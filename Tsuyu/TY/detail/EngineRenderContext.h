#pragma once
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

        void CloseAndFlush();

        const RenderTarget& GetBackBuffer();

        [[nodiscard]]
        ID3D12Device* GetDevice();

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCommandList();

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCopyCommandList();

        Size GetSceneSize();
    }
}
