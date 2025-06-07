#pragma once

#include <d3d12.h>

#include "IEngineUpdatable.h"
#include "TY/RenderTarget.h"
#include "TY/Value2D.h"

namespace TY::detail
{
    class EngineCore_impl
    {
    public:
        void Init() const;

        bool IsInFrame() const;

        void BeginFrame() const;

        void EndFrame() const;

        void Destroy() const;

        const RenderTarget& GetBackBuffer() const;

        [[nodiscard]]
        ID3D12Device* GetDevice() const;

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCommandList() const;

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCopyCommandList() const;

        Size GetSceneSize() const;

        void AddUpdatable(const std::weak_ptr<IEngineUpdatable>& updatable) const;
    };

    inline constexpr auto EngineCore = EngineCore_impl{};
}
