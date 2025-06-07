#include "pch.h"
#include "ConstantBuffer.h"

#include "AssertObject.h"
#include "Utils.h"
#include "detail/EngineCore.h"

using namespace TY;
using namespace TY::detail;

namespace
{
}

struct ConstantBuffer_impl::Impl
{
    uint32_t m_sizeInBytes;
    uint32_t m_count;
    size_t m_alignedSize{};
    ComPtr<ID3D12Resource> m_cb{};

    bool m_uploaded{};
    bool m_shouldUnmap{};

    Impl(uint32_t sizeInBytes, uint32_t count)
        : m_sizeInBytes(sizeInBytes),
          m_count(count)
    {
        const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        m_alignedSize = AlignedSize(sizeInBytes, 256);
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_alignedSize * count);
        AssertWin32{"failed to create commited resource"sv}
            | EngineCore::GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_cb));

        m_cb->SetName(L"ConstantBuffer");
    }

    ~Impl()
    {
        if (m_shouldUnmap)
        {
            m_cb->Unmap(0, nullptr);
        }
    }

    void Upload(const uint8_t* data, uint32_t count)
    {
        uint8_t* dest;
        AssertWin32{"failed to map constant buffer"sv}
            | m_cb->Map(0, nullptr, reinterpret_cast<void**>(&dest));

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
            m_cb->Unmap(0, nullptr);
        }
    }
};

namespace TY
{
    ConstantBuffer_impl::ConstantBuffer_impl(uint32_t sizeInBytes, uint32_t count)
        : p_impl(std::make_shared<Impl>(sizeInBytes, count))
    {
    }

    void ConstantBuffer_impl::upload(const void* data, uint32_t count) const
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(data), count);
    }

    uint32_t ConstantBuffer_impl::count() const
    {
        return p_impl ? p_impl->m_count : 0;
    }

    size_t ConstantBuffer_impl::alignedSize() const
    {
        return p_impl ? p_impl->m_alignedSize : 0;
    }

    uint64_t ConstantBuffer_impl::bufferLocation() const
    {
        return p_impl ? p_impl->m_cb->GetGPUVirtualAddress() : 0;
    }
}
