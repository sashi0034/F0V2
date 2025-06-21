#pragma once
#include "Array.h"

namespace TY
{
    namespace detail
    {
        struct IGpgpuBuffer;
    }

    struct StructuredBufferTransferParams
    {
        int elementCount;
        int elementStride;

        static StructuredBufferTransferParams From(const std::shared_ptr<detail::IGpgpuBuffer>& buffer);
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

        static void AfterDispatch(const Array<StructuredBufferTransfer>& list);

        void beforeFlush();

        static void BeforeFlush(const Array<StructuredBufferTransfer>& list);

        void readback(void* dst);
    };
}
