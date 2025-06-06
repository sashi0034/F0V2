#pragma once
#include "Array.h"

namespace TY
{
    class VertexBuffer_impl
    {
    public:
        VertexBuffer_impl() = default;

        VertexBuffer_impl(int sizeInBytes, int strideInBytes);

        void upload(const void* data);

        void commandSet() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    template <typename Vertex>
    class VertexBuffer : public VertexBuffer_impl
    {
    public:
        VertexBuffer() = default;

        VertexBuffer(int count) : VertexBuffer_impl(count * sizeof(Vertex), sizeof(Vertex))
        {
        }

        VertexBuffer(const Array<Vertex>& data) : VertexBuffer(data.size())
        {
            upload(data);
        }

        void upload(const Array<Vertex>& data) { VertexBuffer_impl::upload(data.data()); }
    };
}
