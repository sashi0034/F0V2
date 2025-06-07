#pragma once

#include <d3d12.h>

#include "IEngineUpdatable.h"
#include "TY/RenderTarget.h"
#include "TY/Value2D.h"

namespace TY::detail
{
    namespace EngineCore
    {
        void Init();

        bool IsInFrame();

        void BeginFrame();

        void EndFrame();

        void Shutdown();

        const RenderTarget& GetBackBuffer();

        [[nodiscard]]
        ID3D12Device* GetDevice();

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCommandList();

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCopyCommandList();

        Size GetSceneSize();

        void AddUpdatable(const std::weak_ptr<IEngineUpdatable>& updatable);
    };
}
