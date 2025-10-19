#pragma once
#include "Array.h"

namespace TY
{
    namespace detail
    {
        struct IGpgpuBuffer;
    }

    struct UnorderedStructuredBufferParams
    {
        int elementCount;
        int elementStride;

        static UnorderedStructuredBufferParams From(const std::shared_ptr<detail::IGpgpuBuffer>& buffer);
    };

    class StructuredBuffer
    {
    public:
        StructuredBuffer() = default;

        StructuredBuffer(const UnorderedStructuredBufferParams& params);

        bool isEmpty() const;

        void upload(const void* src);

        int elementCount() const;

        int elementStride() const;

        ID3D12Resource* getBuffer() const;

    protected:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    class UnorderedStructuredBuffer : public StructuredBuffer
    {
    public:
        UnorderedStructuredBuffer() = default;

        UnorderedStructuredBuffer(const UnorderedStructuredBufferParams& params);

        void afterDispatch();

        static void AfterDispatch(const Array<UnorderedStructuredBuffer>& list);

        void beforeFlush();

        static void BeforeFlush(const Array<UnorderedStructuredBuffer>& list);

        void readback(void* dst);
    };
}
