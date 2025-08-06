#pragma once
#include "Array.h"
#include "Empty.h"

namespace TY
{
    class VertexBufferCore
    {
    public:
        VertexBufferCore(Empty_t empty)
        {
        }

        VertexBufferCore(int sizeInBytes, int strideInBytes);

        bool isEmpty() const;

        int count() const;

        void upload(const void* data);

        void upload(const void* data, int count);

        void commandSet() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    template <typename Vertex>
    class VertexBuffer : public VertexBufferCore
    {
    public:
        VertexBuffer(Empty_t empty) : VertexBufferCore(empty)
        {
        }

        VertexBuffer(int count) : VertexBufferCore(count * sizeof(Vertex), sizeof(Vertex))
        {
        }

        VertexBuffer(const Array<Vertex>& data) : VertexBuffer(data.size())
        {
            upload(data);
        }

        void upload(const Array<Vertex>& data) { VertexBufferCore::upload(data.data(), data.size()); }
    };
}
