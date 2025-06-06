#pragma once

namespace TY::detail
{
    enum class CommandListType
    {
        Direct,
        Copy,
    };

    class CommandList
    {
    public:
        CommandList() = default;

        CommandList(CommandListType type);

        void Flush();

        ID3D12GraphicsCommandList* GetCommandList() const;

        ID3D12CommandQueue* GetCommandQueue() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
