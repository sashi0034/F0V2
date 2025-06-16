#pragma once

namespace TY
{
    struct UnorderedAccessTransferParams
    {
        bool isReadonly;
        int elementCount;
        int elementStride;
    };

    class UnorderedAccessTransfer
    {
    public:
        UnorderedAccessTransfer() = default;

        UnorderedAccessTransfer(const UnorderedAccessTransferParams& params);

        void upload(const void* src);

        void readback(void* dst);

        int elementCount() const;

        int elementStride() const;

        ID3D12Resource* getBuffer() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
