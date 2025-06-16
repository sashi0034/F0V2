#pragma once

namespace TY
{
    struct StructuredBufferTransferParams
    {
        bool isReadonly;
        int elementCount;
        int elementStride;
    };

    class StructuredBufferTransfer
    {
    public:
        StructuredBufferTransfer() = default;

        StructuredBufferTransfer(const StructuredBufferTransferParams& params);

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
