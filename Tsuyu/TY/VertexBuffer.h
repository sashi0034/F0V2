#pragma once
#include "Array.h"
#include "Empty.h"

namespace TY
{
    class VertexBufferImpl
    {
    public:
        VertexBufferImpl(Empty_t empty)
        {
        }

        VertexBufferImpl(int sizeInBytes, int strideInBytes);

        bool isEmpty() const;

        int count() const;

        size_t size_in_bytes() const;

        void upload(const void* data);

        void upload(const void* data, int count);

        void commandSet() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    template <typename Vertex>
    class VertexBuffer : public VertexBufferImpl
    {
    public:
        VertexBuffer(Empty_t empty) : VertexBufferImpl(empty)
        {
        }

        VertexBuffer(int count) : VertexBufferImpl(count * sizeof(Vertex), sizeof(Vertex))
        {
        }

        VertexBuffer(const Array<Vertex>& data) : VertexBuffer(data.size())
        {
            upload(data);
        }

        void upload(const Array<Vertex>& data) { VertexBufferImpl::upload(data.data(), data.size()); }
    };
}
