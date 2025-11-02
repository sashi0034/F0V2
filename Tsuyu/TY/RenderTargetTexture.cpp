#include "pch.h"
#include "RenderTargetTexture.h"

#include "Logger.h"
#include "detail/RenderContext_singleton.h"

using namespace TY;
using namespace TY::detail;

struct RenderTargetTexture::Impl
{
    bool m_valid{};

    RenderTargetTextureParams m_params{};

    TextureHandle m_textureHandle{};

    Impl(const RenderTargetTextureParams& params, bool allowUav)
        : m_params(params)
    {
        CD3DX12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Width = params.size.x;
        resourceDesc.Height = params.size.y;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = params.format;
        resourceDesc.SampleDesc = {1, 0};
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (allowUav)
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = params.format;
        clearValue.Color[0] = m_params.clearColor.r;
        clearValue.Color[1] = m_params.clearColor.g;
        clearValue.Color[2] = m_params.clearColor.b;
        clearValue.Color[3] = m_params.clearColor.a;

        const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        const auto device = RenderContext_singleton::GetDevice();
        if (const HRESULT hr = device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_PRESENT,
                &clearValue,
                IID_PPV_ARGS(m_textureHandle.assignResourceAddress(D3D12_RESOURCE_STATE_PRESENT)));
            FAILED(hr))
        {
            LogError(std::format("RenderTargetTexture: Failed to create texture resource: {:08x}", hr));
            return;
        }

        m_textureHandle.getResource()->SetName(L"RenderTargetTexture::m_textureHandle");

        m_valid = true;
    }

    ~Impl()
    {
    }
};

namespace TY
{
    RenderTargetTextureParams& RenderTargetTextureParams::setSize(Size size_)
    {
        size = size_;
        return *this;
    }

    RenderTargetTextureParams& RenderTargetTextureParams::setClearColor(const ColorF32& clearColor_)
    {
        clearColor = clearColor_;
        return *this;
    }

    RenderTargetTextureParams& RenderTargetTextureParams::setFormat(GraphicsFormat format_)
    {
        format = format_;
        return *this;
    }

    RenderTargetTexture::RenderTargetTexture(const RenderTargetTextureParams& params)
        : p_impl(std::make_shared<Impl>(params, false))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    bool RenderTargetTexture::isEmpty() const
    {
        return p_impl == nullptr;
    }

    Size RenderTargetTexture::size() const
    {
        return p_impl ? p_impl->m_params.size : Size{0, 0};
    }

    ColorF32 RenderTargetTexture::clearColor() const
    {
        return p_impl ? p_impl->m_params.clearColor : ColorF32{0.0f, 0.0f, 0.0f, 0.0f};
    }

    RenderTargetTexture::operator TextureHandle() const
    {
        return p_impl ? p_impl->m_textureHandle : TextureHandle{};
    }

    UnorderedRenderTargetTexture::UnorderedRenderTargetTexture(const RenderTargetTextureParams& params)
        : RenderTargetTexture()
    {
        p_impl = std::make_shared<Impl>(params, true);

        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    UnorderedRenderTargetTexture::operator UnorderedTextureHandle() const
    {
        return p_impl ? UnorderedTextureHandle{p_impl->m_textureHandle} : UnorderedTextureHandle{};
    }
}
