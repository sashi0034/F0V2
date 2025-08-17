#pragma once
#include "TY/Array.h"

namespace TY::detail
{
    enum class CommandListType
    {
        Draw,
        Copy,
        Compute,
    };

    class CommandList
    {
    public:
        CommandList() = default;

        CommandList(CommandListType type);

        void CloseAndFlush();

        /// @brief 指定したコマンドリストにおける最後に実行したコマンドが終わったあとに Close と Flush を行う
        void CloseAndFlushAfter(const CommandList& lastCommandList);

        ID3D12GraphicsCommandList* GetCommandList() const;

        ID3D12CommandQueue* GetCommandQueue() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
