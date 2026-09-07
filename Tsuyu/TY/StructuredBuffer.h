#pragma once
#include "Array.h"

namespace TY
{
    class StructuredBufferObject
    {
    public:
        StructuredBufferObject() = default;

        StructuredBufferObject(int elementCount, int elementStride);

        bool isEmpty() const;

        void upload(const void* src, int count);

        int elementCount() const;

        int elementStride() const;

        ID3D12Resource* getBuffer() const;

    protected:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    class UnorderedStructuredBufferObject : public StructuredBufferObject
    {
    public:
        UnorderedStructuredBufferObject() = default;

        UnorderedStructuredBufferObject(int elementCount, int elementStride);

        void afterDispatch();

        void beforeFlush();

        void readback(void* dst);
    };

    template <typename T>
    class StructuredBuffer : public StructuredBufferObject
    {
    public:
        StructuredBuffer() = default;

        StructuredBuffer(int elementCount)
            : StructuredBufferObject(elementCount, sizeof(T))
        {
        }

        void upload(const Array<T>& data)
        {
            StructuredBufferObject::upload(data.data(), data.size());
        }
    };
}
