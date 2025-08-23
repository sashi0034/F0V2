#include "pch.h"
#include "ShapeDrawer_DescriptorManager.h"

namespace
{
}

namespace TY::ShapeDrawer_detail
{
    DescriptorManager::heap_type DescriptorManager::heap_type::Create(const key_type& key, int cb0_capacity)
    {
        heap_type heap{};

        heap.cbv0 = ConstantBuffer<ShapeDraw_b0>(cb0_capacity);
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
}
