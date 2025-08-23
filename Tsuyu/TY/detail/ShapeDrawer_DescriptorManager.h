#pragma once
#include "ShapeDrawer_Common.h"
#include "TY/Mat3x2.h"

namespace TY::ShapeDrawer_detail
{
    class SD_DescriptorManager
    {
    public:
        struct heap_type
        {
            DescriptorHeap descriptorHeap{};
            DescriptorTable table{};

            ConstantBuffer<ShapeDrawer_b0> cbv0{};
            Array<ShapeDrawer_b0> cb0_value{};
            int next_cb0{};

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
                return next_cb0 >= cbv0.materialCount();
            }

            void resetSrv0(const TextureResource& srv)
            {
                constexpr int tableId = 1;
                keyResource.srv0 = srv;
                descriptorHeap.resetSrv(srv, tableId, 0);
            }

            static constexpr int DefaultCapacity = 4;

            static heap_type Create(const key_type& key, int cb0_capacity = DefaultCapacity);
        };

        struct element_pointer
        {
            int heapIndex{-1};
            int cb0_index{-1};

            bool isValid() const
            {
                return heapIndex >= 0;
            }

            bool operator ==(const element_pointer& other) const
            {
                return std::memcmp(this, &other, sizeof(element_pointer)) == 0;
            }

            bool operator !=(const element_pointer& other) const { return not(*this == other); }
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

        const element_pointer& CurrentPointer() const
        {
            return m_currentPointer;
        }

        const heap_type& CurrentHeap() const
        {
            return m_heapList[m_currentPointer.heapIndex];
        }

        void CommandSet(const element_pointer& element) const;

    private:
        Array<heap_type> m_heapList{};
        element_pointer m_currentPointer{};

        heap_type& currentHeap()
        {
            return m_heapList[m_currentPointer.heapIndex];
        }

        element_pointer fetchHeap(const heap_type::key_type& keyResource);

        element_pointer pushBackNewHeap(const heap_type::key_type& keyResource, int cb0_capacity);
    };
}
