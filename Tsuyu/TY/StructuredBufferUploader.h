#pragma once

namespace TY
{
    struct StructuredBufferTransferParams
    {
        int elementCount;
        int elementStride;
    };

    class StructuredBufferUploader
    {
    public:
        StructuredBufferUploader() = default;

        StructuredBufferUploader(const StructuredBufferTransferParams& params);

        void upload(const void* src);

        int elementCount() const;

        int elementStride() const;

        ID3D12Resource* getBuffer() const;

    protected:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    class StructuredBufferTransfer : public StructuredBufferUploader
    {
    public:
        StructuredBufferTransfer() = default;

        StructuredBufferTransfer(const StructuredBufferTransferParams& params);

        void afterDispatch();

        void beforeFlush();

        void readback(void* dst);
    };
}
