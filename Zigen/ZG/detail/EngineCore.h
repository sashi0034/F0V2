#pragma once

#include <d3d12.h>

#include "IEngineUpdatable.h"
#include "ZG/Value2D.h"

namespace ZG::detail
{
    class EngineCore_impl
    {
    public:
        void Init() const;

        bool IsInFrame() const;

        void BeginFrame() const;

        void EndFrame() const;

        void Destroy() const;

        [[nodiscard]]
        ID3D12Device* GetDevice() const;

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCommandList() const;

        [[nodiscard]]
        ID3D12GraphicsCommandList* GetCopyCommandList() const;

        void FlushCopyCommandList() const;

        Size GetSceneSize() const;

        void AddUpdatable(const std::weak_ptr<IEngineUpdatable>& updatable) const;
    };

    inline constexpr auto EngineCore = EngineCore_impl{};
}
