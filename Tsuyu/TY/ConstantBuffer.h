#pragma once
#include <span>

#include "Array.h"
#include "Empty.h"

namespace TY
{
    class ConstantBufferCore
    {
    public:
        ConstantBufferCore(Empty_t)
        {
        }

        ConstantBufferCore(uint32_t sizeInBytes, uint32_t materialCount = 1);

        bool isEmpty() const;

        void upload(const void* data, uint32_t materialCount = 1) const;

        // TODO: Remove this
        void uploadToDraw(const void* data, uint32_t materialCount = 1) const;

        uint32_t materialCount() const;

        size_t sizeInBytes() const;

        size_t alignedSize() const;

        uint64_t bufferLocation() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    template <typename T>
    class ConstantBuffer : public ConstantBufferCore
    {
    public:
        static constexpr uint32_t sizeInBytes = sizeof(T);

        ConstantBuffer(Empty_t) : ConstantBufferCore(Empty)
        {
        }

        ConstantBuffer(int materialCount = 1) : ConstantBufferCore(sizeInBytes, materialCount)
        {
        }

        ConstantBuffer(const T& data) : ConstantBuffer(data.size())
        {
            upload(data);
        }

        ConstantBuffer(const Array<T>& data) : ConstantBuffer(data.size())
        {
            upload(data);
        }

        void upload(const T& data) const
        {
            ConstantBufferCore::upload(&data, 1);
        }

        void upload(const Array<T>& data) const
        {
            ConstantBufferCore::upload(data.data(), data.size());
        }

        void upload(std::span<const T> data) const
        {
            ConstantBufferCore::upload(data.data(), static_cast<uint32_t>(data.size()));
        }

        void uploadToDraw(const T& data) const
        {
            ConstantBufferCore::uploadToDraw(&data, 1);
        }

        void uploadToDraw(const Array<T>& data) const
        {
            ConstantBufferCore::uploadToDraw(data.data(), data.size());
        }

        void uploadToDraw(std::span<const T> data) const
        {
            ConstantBufferCore::uploadToDraw(data.data(), static_cast<uint32_t>(data.size()));
        }
    };
}
