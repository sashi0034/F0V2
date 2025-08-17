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

        static void SequenceCloseAndFlush(const Array<CommandList>& list);

        ID3D12GraphicsCommandList* GetCommandList() const;

        ID3D12CommandQueue* GetCommandQueue() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
