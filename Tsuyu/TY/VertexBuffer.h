#pragma once
#include "Array.h"

namespace TY
{
    class VertexBufferCore
    {
    public:
        VertexBufferCore() = default;

        VertexBufferCore(int sizeInBytes, int strideInBytes);

        bool isEmpty() const;

        void upload(const void* data);

        void commandSet() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    template <typename Vertex>
    class VertexBuffer : public VertexBufferCore
    {
    public:
        VertexBuffer() = default;

        VertexBuffer(int count) : VertexBufferCore(count * sizeof(Vertex), sizeof(Vertex))
        {
        }

        VertexBuffer(const Array<Vertex>& data) : VertexBuffer(data.size())
        {
            upload(data);
        }

        void upload(const Array<Vertex>& data) { VertexBufferCore::upload(data.data()); }
    };
}
