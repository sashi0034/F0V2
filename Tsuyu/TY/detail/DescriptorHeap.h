#pragma once
#include "DescriptorTable.h"
#include "TY/ShaderResourceTexture.h"
#include "TY/ConstantBufferUploader.h"

namespace TY::detail
{
    struct UnorderedAccessResource
    {
        // TODO
    };

    struct CbSrUaSet
    {
        Array<ConstantBufferUploader_impl> cb; // Array::size() = cbvCount, (ConstantBuffer::count = materialCount)
        Array<Array<ShaderResourceTexture>> sr; // srvCount * materialCount
        Array<Array<UnorderedAccessResource>> ua; // uavCount * materialCount
    };

    struct DescriptorHeapParams
    {
        DescriptorTable table;
        Array<size_t> materialCounts;
        Array<CbSrUaSet> descriptors;
    };

    class DescriptorHeap
    {
    public:
        DescriptorHeap() = default;

        DescriptorHeap(const DescriptorHeapParams& params);

        void CommandSet() const;

        void CommandSetTable(int tableId, int materialId = 0) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
