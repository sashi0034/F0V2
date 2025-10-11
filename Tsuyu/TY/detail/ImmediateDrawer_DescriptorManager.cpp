#include "pch.h"
#include "ImmediateDrawer_DescriptorManager.h"

#include "EngineRenderContext.h"

namespace
{
}

namespace TY::ImmediateDrawer_detail
{
    ID_DescriptorManager::heap_type ID_DescriptorManager::heap_type::Create(const key_type& key, int cbv1_capacity)
    {
        heap_type heap{};

        auto&& cbv0 = EngineRenderContext::GetSceneState3D_CB0();

        heap.cbv1 = ConstantBuffer<ImmediateDrawer_b1>(cbv1_capacity);
        heap.cbv1_value.resize(cbv1_capacity);

        heap.keyResource = key;

        const DescriptorTable descriptorTable = {{1, 1, 0}, {1, 0, 0}};

        heap.descriptorHeap = DescriptorHeap({
            .table = descriptorTable,
            .materialCounts = {1, cbv1_capacity},
            .descriptors = {
                CbvSrvUavSet{{cbv0}, {{heap.keyResource.srv0}}, {}},
                CbvSrvUavSet{{heap.cbv1}, {}, {}},
            }
        });

        heap.table = std::move(descriptorTable);

        return heap;
    }

    void ID_DescriptorManager::RequestTransform(const Mat3x2& transform)
    {
        bool hasChanged;
        if (m_currentCursor.cb1_index == -1)
        {
            hasChanged = true;
        }
        else
        {
            const auto& current_cbv1 = currentHeap().cbv1_value[m_currentCursor.cb1_index];
            hasChanged =
                current_cbv1.g_transform[0].x != transform._11 ||
                current_cbv1.g_transform[0].y != transform._12 ||
                current_cbv1.g_transform[1].x != transform._21 ||
                current_cbv1.g_transform[1].y != transform._22 ||
                current_cbv1.g_transform[0].z != transform._31 ||
                current_cbv1.g_transform[0].w != transform._32;
        }

        if (hasChanged)
        {
            {
                auto& heap = currentHeap();
                if (heap.next_cbv1 >= heap.cbv1.materialCount())
                {
                    m_currentCursor = pushBackNewHeap(heap.keyResource, heap.next_cbv1 * 2);
                }

                heap.next_cbv1++;
            }

            auto& heap = currentHeap();
            m_currentCursor.cb1_index = heap.next_cbv1;
            heap.cbv1_value[m_currentCursor.cb1_index] = {
                .g_transform = {
                    {transform._11, transform._12, transform._31, transform._32},
                    {transform._21, transform._22, 0.0f, 1.0f}
                }
            };
        }
    }

    void ID_DescriptorManager::RequestSrv0(const TextureResource& srv)
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
        m_currentCursor = fetchHeap(newKey);
    }

    void ID_DescriptorManager::Upload() const
    {
        for (int i = 0; i < m_heapList.size(); ++i)
        {
            auto& heap = m_heapList[i];
            if (heap.next_cbv1 > 0)
            {
                heap.cbv1.upload(heap.cbv1_value);
            }
        }
    }

    void ID_DescriptorManager::Reset()
    {
        m_currentCursor = element_cursor{.heapIndex = 0, .cb1_index = -1};

        for (int i = 0; i < m_heapList.size(); ++i)
        {
            m_heapList[i].next_cbv1 = 0;
        }
    }

    void ID_DescriptorManager::CommandSet(const element_cursor& element) const
    {
        auto& heap = m_heapList[element.heapIndex];
        heap.descriptorHeap.commandSet(PipelineType::Graphics);
        heap.descriptorHeap.commandSetTable(PipelineType::Graphics, 0);
        heap.descriptorHeap.commandSetTable(PipelineType::Graphics, 1, element.cb1_index);
    }

    ID_DescriptorManager::element_cursor ID_DescriptorManager::fetchHeap(const heap_type::key_type& keyResource)
    {
        int next_cbv1_capacity = heap_type::DefaultCapacity;
        for (int i = 0; i < m_heapList.size(); ++i)
        {
            if (m_heapList[i].keyResource == keyResource)
            {
                if (not m_heapList[i].isFull())
                {
                    return element_cursor{i, m_heapList[i].next_cbv1};
                }
                else
                {
                    next_cbv1_capacity = Max(next_cbv1_capacity, m_heapList[i].next_cbv1 * 2);
                }
            }
        }

        return pushBackNewHeap(keyResource, next_cbv1_capacity);
    }

    ID_DescriptorManager::element_cursor ID_DescriptorManager::pushBackNewHeap(
        const heap_type::key_type& keyResource,
        int cbv1_capacity)
    {
        m_heapList.push_back(heap_type::Create(keyResource, cbv1_capacity));
        m_heapList.back().keyResource = keyResource;
        return element_cursor{static_cast<int>(m_heapList.size()) - 1, -1};
    }
}
