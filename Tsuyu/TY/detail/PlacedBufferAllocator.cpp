#include "pch.h"
#include "PlacedBufferAllocator.h"
#include "RenderContext_singleton.h"

namespace TY::detail
{
    namespace
    {
        constexpr UINT64 StandardHeapSize = 16 * 1024 * 1024;

        bool alignUp(UINT64 value, UINT64 alignment, UINT64& result)
        {
            if (alignment == 0) return false;
            const UINT64 remainder = value % alignment;
            const UINT64 padding = remainder == 0 ? 0 : alignment - remainder;
            if (value > std::numeric_limits<UINT64>::max() - padding) return false;
            result = value + padding;
            return true;
        }
    }

    struct PlacedBufferHeap
    {
        struct Range
        {
            UINT64 offset;
            UINT64 size;
        };

        ComPtr<ID3D12Heap> heap;
        UINT64 size{};
        bool dedicated{};
        size_t allocationCount{};
        std::vector<Range> freeRanges;

        void release(UINT64 offset, UINT64 length)
        {
            // 割り当て時に返却用の容量も予約し、デストラクタ内でのメモリ確保を避ける。
            const auto position = std::lower_bound(freeRanges.begin(), freeRanges.end(), offset,
                                                   [](const Range& range, UINT64 value)
                                                   {
                                                       return range.offset < value;
                                                   });
            auto current = freeRanges.insert(position, Range{offset, length});
            if (current != freeRanges.begin())
            {
                auto previous = current - 1;
                if (previous->offset + previous->size == current->offset)
                {
                    previous->size += current->size;
                    current = freeRanges.erase(current) - 1;
                }
            }
            if (current + 1 != freeRanges.end() && current->offset + current->size == (current + 1)->offset)
            {
                current->size += (current + 1)->size;
                freeRanges.erase(current + 1);
            }
            --allocationCount;
        }
    };

    struct PlacedBufferAllocatorState
    {
        ComPtr<ID3D12Device> device;
        D3D12_HEAP_TYPE heapType{};
        std::vector<std::shared_ptr<PlacedBufferHeap>> heaps;

        void trimEmptyHeaps()
        {
            // 次回のアロケーション用に通常ヒープを一個だけキャッシュしておき、それ以外は解放
            bool keptEmpty{};
            std::erase_if(heaps, [&keptEmpty](const auto& heap)
            {
                if (heap->allocationCount != 0) return false;
                if (heap->dedicated || keptEmpty) return true;
                keptEmpty = true;
                return false;
            });
        }
    };

    struct PlacedBufferAllocation::Impl
    {
        std::shared_ptr<PlacedBufferAllocatorState> m_state;
        std::shared_ptr<PlacedBufferHeap> m_heap;
        ComPtr<ID3D12Resource> m_resource;
        UINT64 m_offset{};
        UINT64 m_size{};
        D3D12_RESOURCE_STATES m_resourceState{D3D12_RESOURCE_STATE_COMMON};
        bool m_disposed{};

        ~Impl()
        {
            if (not m_state) return;

            m_resource.Reset();
            m_heap->release(m_offset, m_size);
            m_state->trimEmptyHeaps();
        }
    };

    PlacedBufferAllocation::~PlacedBufferAllocation()
    {
        if ((p_impl && p_impl->m_resource) &&
            p_impl.use_count() == 1 && not p_impl->m_disposed)
        {
            p_impl->m_disposed = true;
            RenderContext_singleton::SafeDisposeRenderResource(p_impl);
        }
    }

    PlacedBufferAllocation& PlacedBufferAllocation::operator=(const PlacedBufferAllocation& other)
    {
        if (this == &other) return *this;

        PlacedBufferAllocation copy(other);
        p_impl.swap(copy.p_impl);
        return *this;
    }

    PlacedBufferAllocation& PlacedBufferAllocation::operator=(PlacedBufferAllocation&& other) noexcept
    {
        if (this == &other) return *this;

        PlacedBufferAllocation moved(std::move(other));
        p_impl.swap(moved.p_impl);
        return *this;
    }

    ID3D12Resource* PlacedBufferAllocation::getResource() const
    {
        return p_impl ? p_impl->m_resource.Get() : nullptr;
    }

    bool PlacedBufferAllocation::isEmpty() const
    {
        return getResource() == nullptr;
    }

    void PlacedBufferAllocation::transitionResourceState(D3D12_RESOURCE_STATES newState) const
    {
        if (not p_impl || not p_impl->m_resource || p_impl->m_resourceState == newState) return;

        const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            p_impl->m_resource.Get(),
            p_impl->m_resourceState,
            newState);
        RenderContext_singleton::TargetCommandList()->ResourceBarrier(1, &barrier);
        p_impl->m_resourceState = newState;
    }

    PlacedBufferAllocator::PlacedBufferAllocator(ID3D12Device* device, D3D12_HEAP_TYPE heapType)
        : m_state(std::make_shared<PlacedBufferAllocatorState>())
    {
        m_state->device = device;
        m_state->heapType = heapType;
    }

    HRESULT PlacedBufferAllocator::createResource(
        const D3D12_RESOURCE_DESC& desc,
        D3D12_RESOURCE_STATES initialState,
        PlacedBufferAllocation& out)
    {
        if (not m_state->device ||
            desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
            desc.Width == 0 ||
            (m_state->heapType != D3D12_HEAP_TYPE_DEFAULT && m_state->heapType != D3D12_HEAP_TYPE_UPLOAD) ||
            (m_state->heapType == D3D12_HEAP_TYPE_UPLOAD && initialState != D3D12_RESOURCE_STATE_GENERIC_READ))
        {
            return E_INVALIDARG;
        }

        const auto info = m_state->device->GetResourceAllocationInfo(0, 1, &desc);
        if (info.SizeInBytes == 0 || info.SizeInBytes == std::numeric_limits<UINT64>::max() || info.Alignment == 0)
        {
            return E_INVALIDARG;
        }

        try
        {
            PlacedBufferAllocation allocation;
            allocation.p_impl = std::make_shared<PlacedBufferAllocation::Impl>();
            std::shared_ptr<PlacedBufferHeap> selected;
            size_t rangeIndex{};
            UINT64 offset{};

            for (const auto& heap : m_state->heaps)
            {
                if (heap->dedicated) continue;
                for (size_t i = 0; i < heap->freeRanges.size(); ++i)
                {
                    const auto& range = heap->freeRanges[i];
                    UINT64 aligned{};
                    if (alignUp(range.offset, info.Alignment, aligned) &&
                        aligned - range.offset <= range.size &&
                        info.SizeInBytes <= range.size - (aligned - range.offset))
                    {
                        selected = heap;
                        rangeIndex = i;
                        offset = aligned;
                        break;
                    }
                }
                if (selected) break;
            }

            if (not selected)
            {
                selected = std::make_shared<PlacedBufferHeap>();
                selected->dedicated = info.SizeInBytes > StandardHeapSize;
                if (not alignUp(std::max(StandardHeapSize, info.SizeInBytes),
                                std::max<UINT64>(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, info.Alignment),
                                selected->size))
                {
                    return E_INVALIDARG;
                }
                D3D12_HEAP_DESC heapDesc{};
                heapDesc.SizeInBytes = selected->size;
                heapDesc.Properties = CD3DX12_HEAP_PROPERTIES(m_state->heapType);
                heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
                if (const HRESULT hr = m_state->device->CreateHeap(&heapDesc, IID_PPV_ARGS(&selected->heap));
                    FAILED(hr))
                {
                    return hr;
                }
                selected->heap->SetName(m_state->heapType == D3D12_HEAP_TYPE_DEFAULT
                                            ? L"PlacedBufferAllocator::Default"
                                            : L"PlacedBufferAllocator::Upload");
                selected->freeRanges.push_back({0, selected->size});
                m_state->heaps.push_back(selected);
            }

            // 分割と全割り当ての返却に必要な容量を先に確保する
            selected->freeRanges.reserve(selected->freeRanges.size() + selected->allocationCount + 2);
            const HRESULT hr = m_state->device->CreatePlacedResource(
                selected->heap.Get(), offset, &desc, initialState, nullptr,
                IID_PPV_ARGS(&allocation.p_impl->m_resource));
            if (FAILED(hr))
            {
                m_state->trimEmptyHeaps();
                return hr;
            }

            const auto range = selected->freeRanges[rangeIndex];
            selected->freeRanges.erase(selected->freeRanges.begin() + rangeIndex);
            if (offset != range.offset)
            {
                selected->freeRanges.insert(selected->freeRanges.begin() + rangeIndex++,
                                            {range.offset, offset - range.offset});
            }
            const UINT64 end = offset + info.SizeInBytes;
            if (end < range.offset + range.size)
            {
                selected->freeRanges.insert(selected->freeRanges.begin() + rangeIndex,
                                            {end, range.offset + range.size - end});
            }
            ++selected->allocationCount;
            allocation.p_impl->m_heap = selected;
            allocation.p_impl->m_offset = offset;
            allocation.p_impl->m_size = info.SizeInBytes;
            allocation.p_impl->m_resourceState = initialState;
            allocation.p_impl->m_state = m_state;
            out = std::move(allocation);
            return S_OK;
        }
        catch (const std::bad_alloc&)
        {
            m_state->trimEmptyHeaps();
            return E_OUTOFMEMORY;
        }
    }

    namespace
    {
        std::unique_ptr<PlacedBufferAllocator> s_default;
        std::unique_ptr<PlacedBufferAllocator> s_upload;
    }

    // RenderContext のデバイス生成後に登録する
    void PlacedBufferAllocator_singleton::Init(ID3D12Device* device)
    {
        s_default = std::make_unique<PlacedBufferAllocator>(device, D3D12_HEAP_TYPE_DEFAULT);
        s_upload = std::make_unique<PlacedBufferAllocator>(device, D3D12_HEAP_TYPE_UPLOAD);
    }

    void PlacedBufferAllocator_singleton::Shutdown()
    {
        s_upload.reset();
        s_default.reset();
    }

    PlacedBufferAllocator& PlacedBufferAllocator_singleton::Default()
    {
        assert(s_default);
        return *s_default;
    }

    PlacedBufferAllocator& PlacedBufferAllocator_singleton::Upload()
    {
        assert(s_upload);
        return *s_upload;
    }
}
