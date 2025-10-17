#include "pch.h"
#include "RenderTarget.h"

#include "Logger.h"
#include "RenderTargetTexture.h"
#include "TextureDrawer.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    Array<RenderTarget> s_renderTargetStack{};
}

struct RenderTarget::Impl
{
    bool m_valid{};

    ColorF32 m_clearColor{1.0f, 0.0f, 1.0f, 1.0f};
    Size m_size{};

    RectF m_viewport{};

    D3D12_CPU_DESCRIPTOR_HANDLE m_lastRtvHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_lastDsvHandle{};

    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap{};
    ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap{};

    TextureHandle m_rtvResource{};
    ComPtr<ID3D12Resource> m_dsvResource{};

    // RenderTargetParams m_params{};

    Impl(const RenderTargetParams& params)
    {
        const auto& rtv = params.rtv;

        m_clearColor = params.clearColor;
        m_size = rtv.size();

        m_viewport.size = m_size;

        m_rtvResource = params.rtv;

        if (not CreateInternal(params))
        {
            return;
        }

        m_valid = true;
    }

    ~Impl()
    {
        EngineRenderContext::SafeDisposeRenderResource(m_rtvDescriptorHeap);
        EngineRenderContext::SafeDisposeRenderResource(m_dsvDescriptorHeap);

        // EngineRenderContext::SafeDisposeRenderResource(m_rtvResource);
        EngineRenderContext::SafeDisposeRenderResource(m_dsvResource);
    }

    bool CreateInternal(const RenderTargetParams& params)
    {
        const auto device = EngineRenderContext::GetDevice();

        {
            CD3DX12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resourceDesc.Width = m_size.x;
            resourceDesc.Height = m_size.y;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_D32_FLOAT; // TODO: Support stencil
            resourceDesc.SampleDesc = {1, 0};
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

            D3D12_CLEAR_VALUE clearValue{};
            clearValue.Format = DXGI_FORMAT_D32_FLOAT;
            clearValue.DepthStencil.Depth = 1.0f;
            clearValue.DepthStencil.Stencil = 0;

            if (const HRESULT hr = device->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_DEPTH_WRITE,
                    &clearValue,
                    IID_PPV_ARGS(m_dsvResource.ReleaseAndGetAddressOf()));
                FAILED(hr))
            {
                LogError(std::format("RenderTarget: Failed to create depth stencil resource: {}", hr));
                return false;
            }
        }

        // -----------------------------------------------

        {
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            heapDesc.NumDescriptors = 1; // TODO: MRT
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            if (const HRESULT hr = device->CreateDescriptorHeap(
                    &heapDesc, IID_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf()));
                FAILED(hr))
            {
                LogError(std::format("RenderTarget: Failed to create descriptor heap: {}", hr));
                return false;
            }

            auto rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

            device->CreateRenderTargetView(
                m_rtvResource.getResource(),
                nullptr,
                rtvHandle);

            rtvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            // -----------------------------------------------

            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            if (const HRESULT hr = device->CreateDescriptorHeap(
                    &heapDesc, IID_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf()));
                FAILED(hr))
            {
                LogError(std::format("RenderTarget: Failed to create descriptor heap: {}", hr));
                return false;
            }

            const auto dsvHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            device->CreateDepthStencilView(
                m_dsvResource.Get(),
                nullptr,
                dsvHandle);
        }

        return true;
    }

    void CommandSetViewportAndScissorsRect() const
    {
        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Draw);

        // ビューポートの設定
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = m_viewport.x;
        viewport.TopLeftY = m_viewport.y;
        viewport.Width = m_viewport.w;
        viewport.Height = m_viewport.h;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        // シザー矩形の設定
        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = scissorRect.left + m_size.x;
        scissorRect.bottom = scissorRect.top + m_size.y;
        commandList->RSSetScissorRects(1, &scissorRect);
    }

    ScopedRenderTarget ScopedBind()
    {
        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Draw);

        const auto resourceBarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
            m_rtvResource.getResource(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &resourceBarrierDesc);

        auto rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

        const auto dsvHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

        commandList->ClearRenderTargetView(rtvHandle, m_clearColor.getPointer(), 0, nullptr);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        m_lastRtvHandle = rtvHandle;
        m_lastDsvHandle = dsvHandle;

        CommandSetViewportAndScissorsRect();

        return ScopedRenderTarget{
            [this, commandList]
            {
                const auto resourceBarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
                    m_rtvResource.getResource(),
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    D3D12_RESOURCE_STATE_PRESENT);
                commandList->ResourceBarrier(1, &resourceBarrierDesc);

                if (s_renderTargetStack[s_renderTargetStack.size() - 1].p_impl.get() != this)
                {
                    LogError("RenderTarget::ScopedBind() is not balanced");
                }

                s_renderTargetStack.pop_back();

                if (s_renderTargetStack.empty())
                {
                    return;
                }

                // -----------------------------------------------

                const auto& prev = s_renderTargetStack[s_renderTargetStack.size() - 1].p_impl;
                commandList->OMSetRenderTargets(1, &prev->m_lastRtvHandle, false, &prev->m_lastDsvHandle);
                prev->CommandSetViewportAndScissorsRect();
            }
        };
    }
};

namespace TY
{
    RenderTargetParams& RenderTargetParams::setRtvAndClearColor(const RenderTargetTexture& rtv_)
    {
        rtv = rtv_;
        clearColor = rtv_.clearColor();
        return *this;
    }

    RenderTargetParams& RenderTargetParams::setRtvAndClearColor(const RtvParams& rtv_)
    {
        return setRtvAndClearColor(RenderTargetTexture(rtv_));
    }

    RenderTargetParams& RenderTargetParams::setRtvAndClearColor_unsafe(
        const TextureHandle& rtv_, const ColorF32& clearColor_)
    {
        rtv = rtv_;
        clearColor = clearColor_;

        return *this;
    }

    RenderTarget::RenderTarget(const RenderTargetParams& params)
        : p_impl(std::make_shared<Impl>(params))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    bool RenderTarget::isEmpty() const
    {
        return p_impl == nullptr;
    }

    size_t RenderTarget::unique_id() const
    {
        return p_impl ? reinterpret_cast<size_t>(p_impl.get()) : 0;
    }

    Size RenderTarget::size() const
    {
        return p_impl ? p_impl->m_size : Size{};
    }

    void RenderTarget::setViewport(const RectF& viewport)
    {
        if (p_impl) p_impl->m_viewport = viewport;
    }

    RectF RenderTarget::getViewport() const
    {
        return p_impl ? p_impl->m_viewport : RectF{};
    }

    ScopedRenderTarget RenderTarget::scopedBind() const
    {
        if (not p_impl) return ScopedRenderTarget{};

        s_renderTargetStack.push_back(*this);
        return p_impl->ScopedBind();
    }

    TextureHandle RenderTarget::asTexture() const
    {
        return p_impl->m_rtvResource;
    }

    RenderTarget RenderTarget::Current()
    {
        return s_renderTargetStack.empty()
                   ? RenderTarget{}
                   : s_renderTargetStack[s_renderTargetStack.size() - 1];
    }
}
