#pragma once
#include "Array.h"
#include "Empty.h"

namespace TY
{
    // TODO: Placeholder を分離
    class IndexBuffer
    {
    public:
        using index_type = uint16_t;

        IndexBuffer(Empty_t empty)
        {
        }

        IndexBuffer(int count);

        IndexBuffer(const Array<index_type>& indices);

        void upload(const Array<index_type>& indices);

        void commandSet() const;

        [[nodiscard]]
        int count() const;

        bool resize(int count);

        /// @brief 実態が存在しない IndexBuffer を生成する
        /// @remark VS で SV_VertexID のみ使用する場合に用いることで無駄なバッファを削減出来る
        static IndexBuffer Placeholder(int count);

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
