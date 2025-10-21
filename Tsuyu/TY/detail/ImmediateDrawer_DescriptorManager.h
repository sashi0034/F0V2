#pragma once
#include "ImmediateDrawer_Common.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Mat3x2.h"

namespace TY::ImmediateDrawer_detail
{
    class ID_DescriptorManager
    {
    public:
        //                    +-- heap_type [] --+
        //                    |                  |
        //                    |   +----------+   |
        // element_cursor --> |   | key_type |   |
        //                    |   +----------+   |
        //                    |                  |
        //                    +------------------+

        /// @brief 実際のリソースが格納されたヒープ
        struct heap_type
        {
            DescriptorHeap descriptorHeap{};
            DescriptorTable table{};

            ConstantBufferWrapper<ImmediateDrawer_b1> cbv1{};

            int timeToLive{0};

            /// @brief テクスチャといったリソースはそれぞれ別々のヒープごとを割り当てる。このような特殊リソースはこのクラスにまとめる
            struct key_type
            {
                TextureHandle srv0{};

                bool operator ==(const key_type& other) const
                {
                    return srv0.unique_id() == other.srv0.unique_id();
                }
            } keyResource{};

            void resetSrv0(const TextureHandle& srv)
            {
                constexpr int tableId = 0;
                keyResource.srv0 = srv;
                descriptorHeap.registerSrv(srv, tableId, 0);
            }

            static constexpr int DefaultCapacity = 4;

            static heap_type Create(const key_type& key);
        };

        /// @brief heap_type 配列中の要素を識別するためのクラス。SD_StateManager の状態変化の検知処理で使用する
        struct element_cursor
        {
            int heapIndex{-1};

            bool isValid() const
            {
                return heapIndex >= 0;
            }

            bool operator ==(const element_cursor& other) const
            {
                return std::memcmp(this, &other, sizeof(element_cursor)) == 0;
            }

            bool operator !=(const element_cursor& other) const { return not(*this == other); }
        };

        ID_DescriptorManager();

        void RequestTransform(const Mat3x2& transform);

        void RequestSrv0(const TextureHandle& srv);

        void AfterPresent();

        // FIXME: API 洗練
        void CommitCurrentHeap();

        const element_cursor& CurrentCursor() const;

        const heap_type& CurrentHeap() const;

        void CommandSet(const element_cursor& element) const;

    private:
        Array<heap_type> m_heapList{};
        element_cursor m_currentCursor{};

        void reset();

        heap_type& currentHeap();

        element_cursor fetchHeap(const heap_type::key_type& keyResource);

        element_cursor pushBackNewHeap(const heap_type::key_type& keyResource);
    };
}
