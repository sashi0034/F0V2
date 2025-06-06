#pragma once
#include "Array.h"

namespace TY
{
    class IndexBuffer
    {
    public:
        using index_type = uint16_t;

        IndexBuffer();

        IndexBuffer(int count);

        IndexBuffer(const Array<index_type>& indices);

        void upload(const Array<index_type>& indices);

        void commandSet() const;

        int count() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
