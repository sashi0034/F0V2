#include "pch.h"
#include "ConstantBufferUploader.h"

#include "Logger.h"
#include "Utils.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
}

struct ConstantBufferUploader_impl::Impl
{
    bool m_valid{};

    uint32_t m_sizeInBytes;
    uint32_t m_count;
    size_t m_alignedSize{};
    ComPtr<ID3D12Resource> m_uploadBuffer{};

    bool m_uploaded{};
    bool m_shouldUnmap{};

    Impl(uint32_t sizeInBytes, uint32_t count)
        : m_sizeInBytes(sizeInBytes),
          m_count(count)
    {
        const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        m_alignedSize = AlignedSize(sizeInBytes, 256);
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_alignedSize * count);

        if (const auto hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_uploadBuffer));
            FAILED(hr))
        {
            LogError.writeln("ConstantBufferUploader: Failed to create resource.");
            return;
        }

        m_uploadBuffer->SetName(L"ConstantBuffer");

        m_valid = true;
    }

    ~Impl()
    {
        if (m_shouldUnmap)
        {
            m_uploadBuffer->Unmap(0, nullptr);
        }
    }

    void Upload(const uint8_t* data, uint32_t count)
    {
        uint8_t* dest;

        if (const auto hr = m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dest));
            FAILED(hr))
        {
            LogError.writeln(std::format("ConstantBufferUploader: Failed to map resource for 0x{:016x}",
                                         reinterpret_cast<size_t>(data)));
            return;
        }

        uint32_t srcOffset{};
        for (int i = 0; i < count; ++i)
        {
            std::memcpy(dest, data + srcOffset, m_sizeInBytes);
            srcOffset += m_sizeInBytes;
            dest += m_alignedSize;
        }

        if (m_uploaded)
        {
            // ニ回目以降のアップロードは破棄時までマップの解除をしない
            m_shouldUnmap = true;
        }
        else
        {
            // 初回のアップロードのみマップの解除を行う
            m_uploaded = true;
            m_uploadBuffer->Unmap(0, nullptr);
        }
    }
};

namespace TY
{
    ConstantBufferUploader_impl::ConstantBufferUploader_impl(uint32_t sizeInBytes, uint32_t count)
        : p_impl(std::make_shared<Impl>(sizeInBytes, count))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    bool ConstantBufferUploader_impl::isEmpty() const
    {
        return not p_impl;
    }

    void ConstantBufferUploader_impl::upload(const void* data, uint32_t count) const
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(data), count);
    }

    uint32_t ConstantBufferUploader_impl::count() const
    {
        return p_impl ? p_impl->m_count : 0;
    }

    size_t ConstantBufferUploader_impl::alignedSize() const
    {
        return p_impl ? p_impl->m_alignedSize : 0;
    }

    uint64_t ConstantBufferUploader_impl::bufferLocation() const
    {
        return p_impl ? p_impl->m_uploadBuffer->GetGPUVirtualAddress() : 0;
    }
}
