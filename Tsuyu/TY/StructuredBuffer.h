#pragma once
#include "Array.h"

namespace TY
{
    class StructuredBuffer
    {
    public:
        StructuredBuffer() = default;

        StructuredBuffer(int elementCount, int elementStride);

        bool isEmpty() const;

        void upload(const void* src, int count);

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

        UnorderedStructuredBuffer(int elementCount, int elementStride);

        void afterDispatch();

        void beforeFlush();

        void readback(void* dst);
    };

    // TODO: Rename
    template <typename T>
    class StructuredBufferT : public StructuredBuffer
    {
    public:
        StructuredBufferT() = default;

        StructuredBufferT(int elementCount)
            : StructuredBuffer(elementCount, sizeof(T))
        {
        }

        void upload(const Array<T>& data)
        {
            StructuredBuffer::upload(data.data(), data.size());
        }
    };
}
