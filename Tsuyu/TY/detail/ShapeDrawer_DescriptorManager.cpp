#include "pch.h"
#include "ShapeDrawer_DescriptorManager.h"

namespace
{
}

namespace TY::ShapeDrawer_detail
{
    SD_DescriptorManager::heap_type SD_DescriptorManager::heap_type::Create(const key_type& key, int cb0_capacity)
    {
        heap_type heap{};

        heap.cbv0 = ConstantBuffer<ShapeDrawer_b0>(cb0_capacity);
        heap.cb0_value.resize(cb0_capacity);

        heap.keyResource = key;

        const DescriptorTable descriptorTable = {{1, 0, 0}, {0, 1, 0}};

        heap.descriptorHeap = DescriptorHeap({
            .table = descriptorTable,
            .materialCounts = {cb0_capacity, 1},
            .descriptors = {
                CbvSrvUavSet{{heap.cbv0}, {}, {}},
                CbvSrvUavSet{{}, {{heap.keyResource.srv0}}, {}}
            }
        });

        heap.table = std::move(descriptorTable);

        return heap;
    }

    void SD_DescriptorManager::RequestTransform(const Mat3x2& transform)
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

    void SD_DescriptorManager::RequestSrv0(const ShaderResourceTexture& srv)
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

    void SD_DescriptorManager::Upload() const
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

    void SD_DescriptorManager::Reset()
    {
        m_currentPointer = element_pointer{.heapIndex = 0, .cb0_index = -1};

        for (int i = 0; i < m_heapList.size(); ++i)
        {
            m_heapList[i].next_cb0 = 0;
        }
    }

    void SD_DescriptorManager::CommandSet(const element_pointer& element) const
    {
        auto& heap = m_heapList[element.heapIndex];
        heap.descriptorHeap.commandSet(PipelineType::Graphics);
        heap.descriptorHeap.commandSetTable(PipelineType::Graphics, 0, element.cb0_index);
        heap.descriptorHeap.commandSetTable(PipelineType::Graphics, 1);
    }

    SD_DescriptorManager::element_pointer SD_DescriptorManager::fetchHeap(const heap_type::key_type& keyResource)
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

    SD_DescriptorManager::element_pointer SD_DescriptorManager::pushBackNewHeap(
        const heap_type::key_type& keyResource,
        int cb0_capacity)
    {
        m_heapList.push_back(heap_type::Create(keyResource, cb0_capacity));
        m_heapList.back().keyResource = keyResource;
        return element_pointer{static_cast<int>(m_heapList.size()) - 1, -1};
    }
}
