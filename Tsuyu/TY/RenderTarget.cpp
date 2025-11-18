#include "pch.h"
#include "RenderTarget.h"

#include "Logger.h"
#include "RenderTargetTexture.h"
#include "TextureDrawer.h"
#include "detail/RenderContext_singleton.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    Array<RenderTarget> s_renderTargetStack{};
}

struct RenderTarget::Impl
{
    bool m_valid{};

    Size m_size{};

    RectF m_viewport{};

    // D3D12_CPU_DESCRIPTOR_HANDLE m_lastRtvHandle{};
    // D3D12_CPU_DESCRIPTOR_HANDLE m_lastDsvHandle{};

    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap{};
    ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap{};

    Array<TextureHandle> m_rtvHandles{};
    Array<ColorF32> m_rtvClearColors{};

    Array<DXGI_FORMAT> m_rtvFormats{};
    Array<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvDescriptorHandles{};

    DepthBufferHandle m_dsvHandle{};

    // RenderTargetParams m_params{};

    Impl(const RenderTargetParams& params)
    {
        const auto& rtvHandles = params.rtvHandles;
        if (rtvHandles.size() == 0)
        {
            LogError("RenderTarget: No RTVs specified");
            return;
        }

        if (rtvHandles.size() != params.rtvClearColors.size())
        {
            LogError("RenderTarget: RTV count and clear color count do not match: {} RTVs, {} clear colors",
                     rtvHandles.size(),
                     params.rtvClearColors.size());
            return;
        }

        const Size firstRtvSize = rtvHandles[0].size();
        for (const auto& rtv : rtvHandles)
        {
            if (rtv.size() != firstRtvSize)
            {
                LogError("RenderTarget: All RTVs must have the same size");
                return;
            }

            m_rtvFormats.push_back(rtv.getFormat());
        }

        m_rtvClearColors = params.rtvClearColors;
        m_size = rtvHandles[0].size();

        m_viewport.size = m_size;

        m_rtvHandles = rtvHandles;

        if (not CreateInternal(params))
        {
            return;
        }

        m_valid = true;
    }

    ~Impl()
    {
        RenderContext_singleton::SafeDisposeRenderResource(m_rtvDescriptorHeap);
        RenderContext_singleton::SafeDisposeRenderResource(m_dsvDescriptorHeap);

        // EngineRenderContext::SafeDisposeRenderResource(m_rtvResource);
        // RenderContext_singleton::SafeDisposeRenderResource(m_dsvHandle);
    }

    bool CreateInternal(const RenderTargetParams& params)
    {
        const auto device = RenderContext_singleton::GetDevice();

        if (not params.depthBufferOpt.isEmpty())
        {
            m_dsvHandle = params.depthBufferOpt;
        }
        else
        {
            CD3DX12_RESOURCE_DESC dsvDesc{};
            dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            dsvDesc.Width = m_size.x;
            dsvDesc.Height = m_size.y;
            dsvDesc.DepthOrArraySize = 1;
            dsvDesc.MipLevels = 1;
            dsvDesc.Format = DXGI_FORMAT_R32_TYPELESS; // TODO: Support stencil
            dsvDesc.SampleDesc = {1, 0};
            dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

            D3D12_CLEAR_VALUE clearValue{};
            clearValue.Format = DXGI_FORMAT_D32_FLOAT;
            clearValue.DepthStencil.Depth = 1.0f;
            clearValue.DepthStencil.Stencil = 0;

            if (const HRESULT hr = device->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &dsvDesc,
                    D3D12_RESOURCE_STATE_DEPTH_WRITE,
                    &clearValue,
                    IID_PPV_ARGS(m_dsvHandle.assignResourceAddress(D3D12_RESOURCE_STATE_DEPTH_WRITE)));
                FAILED(hr))
            {
                LogError(std::format("RenderTarget: Failed to create depth stencil resource: {}", hr));
                return false;
            }

            m_dsvHandle.getResource()->SetName(L"RenderTarget::m_dsvResource");
        }

        // -----------------------------------------------

        {
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            heapDesc.NumDescriptors = params.rtvHandles.size();
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            if (const HRESULT hr = device->CreateDescriptorHeap(
                    &heapDesc, IID_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf()));
                FAILED(hr))
            {
                LogError(std::format("RenderTarget: Failed to create m_rtvDescriptorHeap: {}", hr));
                return false;
            }

            auto rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            for (int i = 0; i < params.rtvHandles.size(); ++i)
            {
                device->CreateRenderTargetView(
                    m_rtvHandles[i].getResource(),
                    nullptr,
                    rtvHandle);

                m_rtvDescriptorHandles.push_back(rtvHandle);
                rtvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            }

            // -----------------------------------------------

            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            if (const HRESULT hr = device->CreateDescriptorHeap(
                    &heapDesc, IID_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf()));
                FAILED(hr))
            {
                LogError(std::format("RenderTarget: Failed to create m_dsvDescriptorHeap: {}", hr));
                return false;
            }

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

            const auto dsvHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            device->CreateDepthStencilView(
                m_dsvHandle.getResource(),
                &dsvDesc,
                dsvHandle);
        }

        return true;
    }

    void CommandSetViewportAndScissorsRect() const
    {
        const auto commandList = RenderContext_singleton::TargetCommandList();

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

    ScopedRenderTarget ScopedBind(bool clear)
    {
        const auto commandList = RenderContext_singleton::TargetCommandList();

        Array<D3D12_RESOURCE_STATES> previousResourceStates(m_rtvHandles.size()); // FIXME: 使用直前で各々使いたい State にすべきかも
        for (int i = 0; i < m_rtvHandles.size(); ++i)
        {
            previousResourceStates[i] = m_rtvHandles[i].getResourceState();
            m_rtvHandles[i].transitionResourceState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        m_dsvHandle.transitionResourceState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

        const auto dsvDescriptorHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

        commandList->OMSetRenderTargets(
            m_rtvDescriptorHandles.size(),
            m_rtvDescriptorHandles.data(),
            false,
            &dsvDescriptorHandle);

        if (clear)
        {
            // ClearRenderTargetView()
            for (int i = 0; i < m_rtvHandles.size(); ++i)
            {
                auto& clearColor = m_rtvClearColors[i];
                commandList->ClearRenderTargetView(m_rtvDescriptorHandles[i], clearColor.getPointer(), 0, nullptr);
            }

            commandList->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        }

        CommandSetViewportAndScissorsRect();

        return ScopedRenderTarget{
            [this, commandList, previousResourceStates]
            {
                for (int i = 0; i < m_rtvHandles.size(); ++i)
                {
                    m_rtvHandles[i].transitionResourceState(previousResourceStates[i]);
                }

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
                // 直前の RTV と DSV を再設定

                const auto& prev = s_renderTargetStack[s_renderTargetStack.size() - 1].p_impl;

                const auto prevDsvDescriptorHandle = prev->m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

                commandList->OMSetRenderTargets(
                    prev->m_rtvDescriptorHandles.size(),
                    prev->m_rtvDescriptorHandles.data(),
                    false,
                    &prevDsvDescriptorHandle);
                prev->CommandSetViewportAndScissorsRect();
            }
        };
    }
};

namespace TY
{
    RenderTargetParams& RenderTargetParams::setRtvList(const Array<RenderTargetTexture>& list)
    {
        rtvHandles.clear();
        rtvClearColors.clear();

        for (const auto& rtv : list)
        {
            rtvHandles.push_back(rtv);
            rtvClearColors.push_back(rtv.clearColor());
        }

        return *this;
    }

    RenderTargetParams& RenderTargetParams::setRtv(const RenderTargetTexture& rtv_)
    {
        rtvHandles = {rtv_};
        rtvClearColors = {rtv_.clearColor()};
        return *this;
    }

    RenderTargetParams& RenderTargetParams::setRtv(const RtvParams& rtv_)
    {
        return setRtv(RenderTargetTexture(rtv_));
    }

    RenderTargetParams& RenderTargetParams::setRtv_unsafe(
        const TextureHandle& rtv_, const ColorF32& clearColor_)
    {
        rtvHandles = {rtv_};
        rtvClearColors = {clearColor_};
        return *this;
    }

    RenderTargetParams& RenderTargetParams::setDepthBuffer(const DepthBufferHandle& depthBuffer_)
    {
        depthBufferOpt = depthBuffer_;
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

    ScopedRenderTarget RenderTarget::scopedClearBind() const
    {
        if (not p_impl)
        {
            return ScopedRenderTarget{};
        }

        s_renderTargetStack.push_back(*this);
        return p_impl->ScopedBind(true);
    }

    ScopedRenderTarget RenderTarget::scopedBind() const
    {
        if (not p_impl)
        {
            return ScopedRenderTarget{};
        }

        s_renderTargetStack.push_back(*this);
        return p_impl->ScopedBind(false);
    }

    TextureHandle RenderTarget::getFrontRtv() const
    {
        return p_impl && not p_impl->m_rtvHandles.empty() ? p_impl->m_rtvHandles[0] : TextureHandle{};
    }

    DepthBufferHandle RenderTarget::getDepthBuffer() const
    {
        return p_impl ? p_impl->m_dsvHandle : DepthBufferHandle{};
    }

    Array<GraphicsFormat> RenderTarget::getRtvFormats() const
    {
        return p_impl ? p_impl->m_rtvFormats : Array<DXGI_FORMAT>{};
    }

    RenderTarget RenderTarget::Current()
    {
        return s_renderTargetStack.empty()
                   ? RenderTarget{}
                   : s_renderTargetStack[s_renderTargetStack.size() - 1];
    }
}
