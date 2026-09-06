#include "pch.h"
#include "IndexBuffer.h"

#include "Logger.h"
#include "detail/RenderContext_singleton.h"
#include "detail/PlacedBufferAllocator.h"

using namespace TY;
using namespace TY::detail;

struct IndexBuffer::Impl
{
    bool m_valid{};

    int m_indexCount{};

    virtual ~Impl() = default;

    virtual void Upload(const Array<index_type>& indices) = 0;

    virtual bool Resize(int count) = 0;

    virtual void CommandSet() const = 0;

    // -----------------------------------------------

    struct Default;

    struct Placeholder;
};

struct IndexBuffer::Impl::Default : Impl
{
    int m_capacity{};

    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

    PlacedBufferAllocation m_bufferAllocation{};

    struct frame_resources
    {
        PlacedBufferAllocation uploadBuffer;
        index_type* dest{};
    };

    std::array<frame_resources, RenderContext_singleton::FrameBufferCount> m_frameResources{};

    size_t m_uploadTimestamp{};

    Default(int count)
    {
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(index_type) * count);

        // リソース作成
        if (const auto hr = PlacedBufferAllocator_singleton::Default().createResource(
                resourceDesc, D3D12_RESOURCE_STATE_COMMON, m_bufferAllocation);
            FAILED(hr))
        {
            LogError.writeln(L"IndexBuffer: Failed to create buffer");
            return;
        }

        m_bufferAllocation.getResource()->SetName(L"IndexBuffer::m_bufferAllocation");

        m_indexBufferView.BufferLocation = m_bufferAllocation.getResource()->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = resourceDesc.Width;
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

        m_capacity = count;
        m_indexCount = count;

        m_valid = true;
    }

    ~Default() override
    {
        for (auto& frameResource : m_frameResources)
        {
            if (not frameResource.uploadBuffer.isEmpty() && frameResource.dest)
            {
                frameResource.uploadBuffer.getResource()->Unmap(0, nullptr);
            }
        }
    }

    void Upload(const Array<index_type>& indices) override
    {
        const size_t previousUploadTimestamp = m_uploadTimestamp;
        m_uploadTimestamp = RenderContext_singleton::GetFlushTimestamp();

        const size_t frameIndex = m_uploadTimestamp % RenderContext_singleton::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        if (frameResource.uploadBuffer.isEmpty())
        {
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferView.SizeInBytes);

            if (const HRESULT hr = PlacedBufferAllocator_singleton::Upload().createResource(
                    resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, frameResource.uploadBuffer);
                FAILED(hr))
            {
                LogError.writeln("IndexBuffer: Failed to create uploadBuffer.");
                return;
            }
        }

        if (not frameResource.dest)
        {
            if (const HRESULT hr = frameResource.uploadBuffer.getResource()->Map(
                    0, nullptr, reinterpret_cast<void**>(&frameResource.dest));
                FAILED(hr))
            {
                LogError.writeln(std::format("IndexBuffer: Failed to map resource for 0x{:016x}",
                                             reinterpret_cast<size_t>(indices.data())));
                return;
            }

            frameResource.uploadBuffer.getResource()->SetName(L"IndexBuffer::uploadBuffer");
        }

        index_type* dest = frameResource.dest;

        std::ranges::copy(indices, dest);

        m_bufferAllocation.transitionResourceState(D3D12_RESOURCE_STATE_COPY_DEST);

        RenderContext_singleton::TargetCommandList()->CopyBufferRegion(
            m_bufferAllocation.getResource(),
            0,
            frameResource.uploadBuffer.getResource(),
            0,
            indices.size_in_bytes());

        m_bufferAllocation.transitionResourceState(D3D12_RESOURCE_STATE_INDEX_BUFFER);

        if (previousUploadTimestamp == 0)
        {
            // 初回実行時は即アンマップする
            frameResource.uploadBuffer.getResource()->Unmap(0, nullptr);
            frameResource.dest = nullptr;
        }
    }

    bool Resize(int count) override
    {
        if (count <= m_capacity)
        {
            m_indexCount = count;
            return true;
        }

        return false;
    }

    void CommandSet() const override
    {
        const auto commandList = RenderContext_singleton::TargetCommandList();
        commandList->IASetIndexBuffer(&m_indexBufferView);
    }
};

struct IndexBuffer::Impl::Placeholder : Impl
{
    Placeholder(int count)
    {
        m_indexCount = count;

        m_valid = true;
    }

    void Upload(const Array<index_type>&) override
    {
        LogError.writeln("IndexBuffer: Upload called on Placeholder implementation.");
    }

    bool Resize(int count)
    {
        m_indexCount = count;
        return true;
    }

    void CommandSet() const override
    {
        LogError.writeln("IndexBuffer: CommandSet called on Placeholder implementation.");
    }
};

namespace TY
{
    IndexBuffer::IndexBuffer(int count) : p_impl(std::make_shared<Impl::Default>(count))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
            return;
        }
    }

    IndexBuffer::IndexBuffer(const Array<index_type>& indices) : p_impl(std::make_shared<Impl::Default>(indices.size()))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
            return;
        }

        p_impl->Upload(indices);
    }

    void IndexBuffer::upload(const Array<index_type>& indices)
    {
        if (not p_impl) return;
        p_impl->Upload(indices);
    }

    void IndexBuffer::commandSet() const
    {
        if (not p_impl) return;
        p_impl->CommandSet();
    }

    int IndexBuffer::count() const
    {
        if (not p_impl) return {};
        return p_impl->m_indexCount;
    }

    bool IndexBuffer::resize(int count)
    {
        return p_impl ? p_impl->Resize(count) : false;
    }

    IndexBuffer IndexBuffer::Placeholder(int count)
    {
        IndexBuffer result{Empty};
        result.p_impl = std::make_shared<Impl::Placeholder>(count);
        return result;
    }
}
