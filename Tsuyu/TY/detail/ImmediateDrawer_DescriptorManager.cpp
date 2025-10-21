#include "pch.h"
#include "ImmediateDrawer_DescriptorManager.h"

#include "EngineRenderContext.h"

namespace
{
    constexpr int maxTimeToLive = 90;
}

namespace TY::ImmediateDrawer_detail
{
    ID_DescriptorManager::heap_type ID_DescriptorManager::heap_type::Create(const key_type& key)
    {
        heap_type heap{};

        auto&& cbv0 = EngineRenderContext::GetSceneState3D_CB0();

        heap.cbv1 = ConstantBufferWrapper<ImmediateDrawer_b1>();

        heap.keyResource = key;

        const DescriptorTable descriptorTable = {{2, 1, 0}};

        heap.descriptorHeap = DescriptorHeap({
            .table = descriptorTable,
            .materialCounts = {1},
            .descriptors = {
                CbvSrvUavSet{{cbv0, heap.cbv1}, {{heap.keyResource.srv0}}, {}},
            }
        });

        heap.table = std::move(descriptorTable);

        return heap;
    }

    ID_DescriptorManager::ID_DescriptorManager()
    {
        reset();
    }

    void ID_DescriptorManager::RequestTransform(const Mat3x2& transform)
    {
        const auto& current_cbv1 = currentHeap().cbv1.value();
        const bool hasChanged =
            current_cbv1.g_transform[0].x != transform._11 ||
            current_cbv1.g_transform[0].y != transform._12 ||
            current_cbv1.g_transform[1].x != transform._21 ||
            current_cbv1.g_transform[1].y != transform._22 ||
            current_cbv1.g_transform[0].z != transform._31 ||
            current_cbv1.g_transform[0].w != transform._32;

        if (hasChanged)
        {
            currentHeap().cbv1.uploadValue({
                .g_transform = {
                    {transform._11, transform._12, transform._31, transform._32},
                    {transform._21, transform._22, 0.0f, 1.0f}
                }
            });
        }
    }

    void ID_DescriptorManager::RequestSrv0(const TextureHandle& srv)
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

    void ID_DescriptorManager::AfterPresent()
    {
        for (auto& heap : m_heapList)
        {
            if (heap.timeToLive > 0)
            {
                --heap.timeToLive;
            }
        }

        for (int i = static_cast<int>(m_heapList.size()) - 1; i >= 0; --i)
        {
            if (m_heapList[i].timeToLive == 0)
            {
                m_heapList.erase(m_heapList.begin() + i);
            }
        }

        reset();
    }

    void ID_DescriptorManager::CommitCurrentHeap()
    {
        currentHeap().timeToLive = maxTimeToLive;
    }

    const ID_DescriptorManager::element_cursor& ID_DescriptorManager::CurrentCursor() const
    {
        return m_currentCursor;
    }

    const ID_DescriptorManager::heap_type& ID_DescriptorManager::CurrentHeap() const
    {
        return m_heapList[m_currentCursor.heapIndex];
    }

    void ID_DescriptorManager::CommandSet(const element_cursor& element) const
    {
        auto& heap = m_heapList[element.heapIndex];
        heap.descriptorHeap.commandSet();
        heap.descriptorHeap.commandSetGraphicsTable(0);
    }

    void ID_DescriptorManager::reset()
    {
        if (m_heapList.empty())
        {
            pushBackNewHeap(heap_type::key_type{});
        }

        m_currentCursor = element_cursor{.heapIndex = 0,};

        currentHeap().timeToLive = maxTimeToLive;
    }

    ID_DescriptorManager::heap_type& ID_DescriptorManager::currentHeap()
    {
        return m_heapList[m_currentCursor.heapIndex];
    }

    ID_DescriptorManager::element_cursor ID_DescriptorManager::fetchHeap(const heap_type::key_type& keyResource)
    {
        for (int i = 0; i < m_heapList.size(); ++i)
        {
            if (m_heapList[i].keyResource == keyResource)
            {
                return element_cursor{i};
            }
        }

        return pushBackNewHeap(keyResource);
    }

    ID_DescriptorManager::element_cursor ID_DescriptorManager::pushBackNewHeap(
        const heap_type::key_type& keyResource)
    {
        m_heapList.push_back(heap_type::Create(keyResource));
        m_heapList.back().keyResource = keyResource;
        return element_cursor{static_cast<int>(m_heapList.size()) - 1};
    }
}
