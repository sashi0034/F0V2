#pragma once
#include "ShapeDrawer_Common.h"
#include "TY/Mat3x2.h"

namespace TY::ShapeDrawer_detail
{
    class DescriptorManager
    {
    public:
        struct heap_type
        {
            DescriptorHeap descriptorHeap{};
            DescriptorTable table{};

            ConstantBuffer<ShapeDraw_b0> cbv0{};
            Array<ShapeDraw_b0> cb0_value{};
            int next_cb0{};

            struct key_type
            {
                ShaderResourceTexture srv0{};

                bool operator ==(const key_type& other) const
                {
                    return srv0.unique_id() == other.srv0.unique_id();
                }
            } keyResource{};

            bool isFull() const
            {
                return next_cb0 >= cbv0.materialCount();
            }

            void resetSrv0(const ShaderResourceTexture& srv)
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

        DescriptorManager()
        {
            pushBackNewHeap(heap_type::key_type{}, heap_type::DefaultCapacity);

            Reset();
        }

        void RequestTransform(const Mat3x2& transform)
        {
            bool isDifferent;
            if (m_currentPointer.cb0_index == -1)
            {
                isDifferent = true;
            }
            else
            {
                const auto& current_cb0 = currentHeap().cb0_value[m_currentPointer.cb0_index];
                isDifferent =
                    current_cb0.g_transform[0].x != transform._11 ||
                    current_cb0.g_transform[0].y != transform._12 ||
                    current_cb0.g_transform[1].x != transform._21 ||
                    current_cb0.g_transform[1].y != transform._22 ||
                    current_cb0.g_transform[0].z != transform._31 ||
                    current_cb0.g_transform[0].w != transform._32;
            }

            if (isDifferent)
            {
                {
                    auto& heap = currentHeap();
                    if (heap.next_cb0 >= heap.cbv0.materialCount())
                    {
                        m_currentPointer = pushBackNewHeap(heap.keyResource, heap.next_cb0 * 2);
                    }

                    heap.next_cb0++;
                }

                auto& heap = currentHeap();
                m_currentPointer.cb0_index = heap.next_cb0;
                heap.cb0_value[m_currentPointer.cb0_index] = {
                    .g_transform = {
                        {transform._11, transform._12, transform._31, transform._32},
                        {transform._21, transform._22, 0.0f, 1.0f}
                    }
                };
            }
        }

        void RequestSrv0(const ShaderResourceTexture& srv)
        {
            if (currentHeap().keyResource.srv0.unique_id() == srv.unique_id())
            {
                return;
            }

            if (currentHeap().keyResource.srv0.isEmpty())
            {
                currentHeap().resetSrv0(srv);
                return;
            }

            auto newKey = currentHeap().keyResource;
            newKey.srv0 = srv;
            m_currentPointer = fetchHeap(newKey);
        }

        void Upload() const
        {
            for (int i = 0; i < m_heapList.size(); ++i)
            {
                auto& heap = m_heapList[i];
                if (heap.next_cb0 > 0)
                {
                    heap.cbv0.upload(heap.cb0_value);
                }
            }
        }

        void Reset()
        {
            m_currentPointer = element_pointer{.heapIndex = 0, .cb0_index = -1};

            for (int i = 0; i < m_heapList.size(); ++i)
            {
                m_heapList[i].next_cb0 = 0;
            }
        }

        const element_pointer& CurrentPointer() const
        {
            return m_currentPointer;
        }

        const heap_type& CurrentHeap() const
        {
            return m_heapList[m_currentPointer.heapIndex];
        }

        void CommandSet(const element_pointer& element) const
        {
            auto& heap = m_heapList[element.heapIndex];
            heap.descriptorHeap.commandSet(PipelineType::Graphics);
            heap.descriptorHeap.commandSetTable(PipelineType::Graphics, 0, element.cb0_index);
            heap.descriptorHeap.commandSetTable(PipelineType::Graphics, 1);
        }

    private:
        Array<heap_type> m_heapList{};
        element_pointer m_currentPointer{};

        heap_type& currentHeap()
        {
            return m_heapList[m_currentPointer.heapIndex];
        }

        element_pointer fetchHeap(const heap_type::key_type& keyResource)
        {
            int next_cb0_capacity = heap_type::DefaultCapacity;
            for (int i = 0; i < m_heapList.size(); ++i)
            {
                if (m_heapList[i].keyResource == keyResource)
                {
                    if (not m_heapList[i].isFull())
                    {
                        return element_pointer{i, m_heapList[i].next_cb0};
                    }
                    else
                    {
                        next_cb0_capacity = Max(next_cb0_capacity, m_heapList[i].next_cb0 * 2);
                    }
                }
            }

            return pushBackNewHeap(keyResource, next_cb0_capacity);
        }

        element_pointer pushBackNewHeap(const heap_type::key_type& keyResource, int cb0_capacity)
        {
            m_heapList.push_back(heap_type::Create(keyResource, cb0_capacity));
            m_heapList.back().keyResource = keyResource;
            return element_pointer{static_cast<int>(m_heapList.size()) - 1, -1};
        }
    };
}
