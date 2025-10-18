#pragma once
#include "TY/Array.h"

namespace TY::detail
{
    enum class CommandListType
    {
        Draw,
        // Copy,
        // Compute,
    };

    class CommandListManager
    {
    public:
        CommandListManager() = default;

        CommandListManager(CommandListType type);

        void closeAndAdvance();

        /// @brief 指定したコマンドリストにおける最後に実行したコマンドが終わったあとに Close と Flush を行う
        // void CloseAndFlushAfter(const CommandList& lastCommandList);

        void waitLastCommandList();

        ID3D12GraphicsCommandList* getCommandList() const;

        ID3D12CommandQueue* getCommandQueue() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
