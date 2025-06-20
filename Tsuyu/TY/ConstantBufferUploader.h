#pragma once
#include "Array.h"
#include "Empty.h"

namespace TY
{
    class ConstantBufferUploader_impl
    {
    public:
        ConstantBufferUploader_impl(Empty_t)
        {
        }

        ConstantBufferUploader_impl(uint32_t sizeInBytes, uint32_t materialCount = 1);

        bool isEmpty() const;

        void upload(const void* data, uint32_t materialCount = 1) const;

        uint32_t materialCount() const;

        size_t sizeInBytes() const;

        size_t alignedSize() const;

        uint64_t bufferLocation() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    template <typename T>
    class ConstantBufferUploader : public ConstantBufferUploader_impl
    {
    public:
        static constexpr uint32_t sizeInBytes = sizeof(T);

        ConstantBufferUploader(Empty_t) : ConstantBufferUploader_impl(Empty)
        {
        }

        ConstantBufferUploader(int materialCount = 1) : ConstantBufferUploader_impl(sizeInBytes, materialCount)
        {
        }

        ConstantBufferUploader(const T& data) : ConstantBufferUploader(data.size())
        {
            upload(data);
        }

        ConstantBufferUploader(const Array<T>& data) : ConstantBufferUploader(data.size())
        {
            upload(data);
        }

        void upload(const T& data) const
        {
            ConstantBufferUploader_impl::upload(&data, 1);
        }

        void upload(const Array<T>& data) const
        {
            ConstantBufferUploader_impl::upload(data.data(), data.size());
        }
    };
}
