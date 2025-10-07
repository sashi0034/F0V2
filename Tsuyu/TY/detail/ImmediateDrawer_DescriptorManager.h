#pragma once
#include "ImmediateDrawer_Common.h"
#include "TY/Mat3x2.h"

namespace TY::ImmediateDrawer_detail
{
    class SD_DescriptorManager
    {
    public:
        //                    --- heap_type [] ---
        //                    |                  |
        //                    |   ------------   |
        // element_cursor --> |   | key_type |   |
        //                    |   ------------   |
        //                    |                  |
        //                    --------------------

        /// @brief 実際のリソースが格納されたヒープ
        struct heap_type
        {
            DescriptorHeap descriptorHeap{};
            DescriptorTable table{};

            ConstantBuffer<ImmediateDrawer_b1> cbv1{};
            Array<ImmediateDrawer_b1> cbv1_value{};
            int next_cbv1{};

            /// @brief テクスチャといったリソースはそれぞれ別々のヒープごとを割り当てる。このような特殊リソースはこのクラスにまとめる
            struct key_type
            {
                TextureResource srv0{};

                bool operator ==(const key_type& other) const
                {
                    return srv0.unique_id() == other.srv0.unique_id();
                }
            } keyResource{};

            bool isFull() const
            {
                return next_cbv1 >= cbv1.materialCount();
            }

            void resetSrv0(const TextureResource& srv)
            {
                constexpr int tableId = 0;
                keyResource.srv0 = srv;
                descriptorHeap.resetSrv(srv, tableId, 0);
            }

            static constexpr int DefaultCapacity = 4;

            static heap_type Create(const key_type& key, int cbv1_capacity = DefaultCapacity);
        };

        /// @brief heap_type 配列中の要素を識別するためのクラス。SD_StateManager の状態変化の検知処理で使用する
        struct element_cursor
        {
            int heapIndex{-1};
            int cb1_index{-1};

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

        SD_DescriptorManager()
        {
            pushBackNewHeap(heap_type::key_type{}, heap_type::DefaultCapacity);

            Reset();
        }

        void RequestTransform(const Mat3x2& transform);

        void RequestSrv0(const TextureResource& srv);

        void Upload() const;

        void Reset();

        const element_cursor& CurrentCursor() const
        {
            return m_currentCursor;
        }

        const heap_type& CurrentHeap() const
        {
            return m_heapList[m_currentCursor.heapIndex];
        }

        void CommandSet(const element_cursor& element) const;

    private:
        Array<heap_type> m_heapList{};
        element_cursor m_currentCursor{};

        heap_type& currentHeap()
        {
            return m_heapList[m_currentCursor.heapIndex];
        }

        element_cursor fetchHeap(const heap_type::key_type& keyResource);

        element_cursor pushBackNewHeap(const heap_type::key_type& keyResource, int cbv1_capacity);
    };
}
