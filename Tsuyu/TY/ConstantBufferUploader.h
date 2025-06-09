#pragma once
#include "Array.h"

namespace TY
{
    class ConstantBufferUploader_impl
    {
    public:
        ConstantBufferUploader_impl() = default;

        ConstantBufferUploader_impl(uint32_t sizeInBytes, uint32_t count);

        void upload(const void* data, uint32_t count) const;

        uint32_t count() const;

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

        ConstantBufferUploader() : ConstantBufferUploader_impl()
        {
        }

        ConstantBufferUploader(int count) : ConstantBufferUploader_impl(sizeInBytes, count)
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
