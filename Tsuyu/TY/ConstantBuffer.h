#pragma once
#include "Empty.h"

namespace TY
{
    class ConstantBufferObject
    {
    public:
        [[nodiscard]]
        ConstantBufferObject(Empty_t)
        {
        }

        [[nodiscard]]
        ConstantBufferObject(uint32_t sizeInBytes);

        void upload(const void* data) const;

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        size_t alignedSize() const;

        [[nodiscard]]
        uint64_t bufferLocation() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    template <class T>
    class ConstantBuffer : public ConstantBufferObject
    {
    public:
        [[nodiscard]]
        ConstantBuffer(Empty_t) : ConstantBufferObject(Empty)
        {
        }

        [[nodiscard]]
        ConstantBuffer() : ConstantBufferObject(sizeof(T))
        {
        }

        void upload(const T& data) const
        {
            ConstantBufferObject::upload(&data);
        }
    };
}
