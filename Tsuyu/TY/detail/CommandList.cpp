#include "pch.h"
#include "CommandList.h"

#include "EngineRenderContext.h"
#include "TY/AssertObject.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    D3D12_COMMAND_LIST_TYPE getCommandListType(CommandListType type)
    {
        switch (type)
        {
        case CommandListType::Direct:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case CommandListType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;
        case CommandListType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        }

        assert(false);
        return {};
    }
}

struct CommandList::Impl
{
    bool m_valid{};

    struct frame_resource
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator{};
        ComPtr<ID3D12GraphicsCommandList> commandList{};
        UINT64 fenceValue{};
        bool needFence{};
    };

    Array<frame_resource> m_frameResources{EngineRenderContext::FrameBufferCount};

    uint8_t m_frameResourceIndex{0};

    ComPtr<ID3D12CommandQueue> m_commandQueue{};

    ComPtr<ID3D12Fence> m_fence{};

    Impl(CommandListType type)
    {
        const auto device = EngineRenderContext::GetDevice();
        const auto commandListType = getCommandListType(type);

        for (int i = 0; i < EngineRenderContext::FrameBufferCount; ++i)
        {
            auto& rsc = m_frameResources[i];

            // コマンドアロケータを生成
            if (const HRESULT hr = device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&rsc.commandAllocator));
                FAILED(hr))
            {
                LogError(std::format("CreateCommandAllocator failed: {}", hr));
                return;
            }

            rsc.commandAllocator->SetName(L"CommandAllocator");

            // コマンドリストを生成
            if (const HRESULT hr = device->CreateCommandList(
                    0,
                    commandListType,
                    rsc.commandAllocator.Get(),
                    nullptr,
                    IID_PPV_ARGS(&rsc.commandList));
                FAILED(hr))
            {
                LogError(std::format("CreateCommandList failed: {}", hr));
                return;
            }

            rsc.commandList->SetName(L"CommandList");

            if (i > 0)
            {
                rsc.commandList->Close();
            }

            rsc.fenceValue = i;
        }

        // コマンドキューを生成
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
        commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // タイムアウトなし
        commandQueueDesc.NodeMask = 0;
        commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // プライオリティ特に指定なし
        commandQueueDesc.Type = commandListType;

        if (const HRESULT hr = device->CreateCommandQueue(
                &commandQueueDesc, IID_PPV_ARGS(&m_commandQueue));
            FAILED(hr))
        {
            LogError(std::format("CreateCommandQueue failed: {}", hr));
            return;
        }

        m_commandQueue->SetName(L"CommandQueue");

        // フェンスを生成
        if (const HRESULT hr = device->CreateFence(
                0,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&m_fence));
            FAILED(hr))
        {
            LogError(std::format("CreateFence failed: {}", hr));
            return;
        }

        m_fence->SetName(L"Fence");

        m_valid = true;
    }

    void CloseAndFlush()
    {
        auto& currentResource = m_frameResources[m_frameResourceIndex];

        currentResource.commandList->Close();

        ID3D12CommandList* commandLists[] = {currentResource.commandList.Get()};
        m_commandQueue->ExecuteCommandLists(1, commandLists);

        // 実行の待機
        currentResource.fenceValue += EngineRenderContext::FrameBufferCount;
        m_commandQueue->Signal(m_fence.Get(), currentResource.fenceValue);

        // -----------------------------------------------

        m_frameResourceIndex = (m_frameResourceIndex + 1) % EngineRenderContext::FrameBufferCount;

        auto& nextResource = m_frameResources[m_frameResourceIndex];

        if (nextResource.needFence && m_fence->GetCompletedValue() < nextResource.fenceValue)
        {
            const auto event = CreateEvent(nullptr, false, false, nullptr);
            m_fence->SetEventOnCompletion(nextResource.fenceValue, event);
            WaitForSingleObjectEx(event, INFINITE, false);
            CloseHandle(event);
        }

        nextResource.needFence = true;

        // コマンドアロケータのリセット
        nextResource.commandAllocator->Reset();

        // コマンドリストのリセット
        nextResource.commandList->Reset(nextResource.commandAllocator.Get(), nullptr);
    }

    static void SequenceCloseAndFlush(const Array<CommandList>& list)
    {
        if (list.empty())
        {
            return;
        }

        std::shared_ptr<Impl> prev = nullptr;
        for (auto& it : list)
        {
            const auto& impl = it.p_impl;
            assert(impl);

            auto& currentResource = impl->m_frameResources[impl->m_frameResourceIndex];

            currentResource.commandList->Close();

            // 前と違うキューなら依存を入れる
            if (prev && prev->m_commandQueue.Get() != impl->m_commandQueue.Get())
            {
                impl->m_commandQueue->Wait(prev->m_fence.Get(),
                                           prev->m_frameResources[prev->m_frameResourceIndex].fenceValue);
            }

            ID3D12CommandList* cmds[] = {currentResource.commandList.Get()};
            impl->m_commandQueue->ExecuteCommandLists(1, cmds);

            currentResource.fenceValue += EngineRenderContext::FrameBufferCount;
            impl->m_commandQueue->Signal(impl->m_fence.Get(), currentResource.fenceValue);

            prev = impl;
        }

        // -----------------------------------------------

        for (int i = list.size() - 1; i >= 0; --i)
        {
            auto& impl = list[i].p_impl;

            impl->m_frameResourceIndex = (impl->m_frameResourceIndex + 1) % EngineRenderContext::FrameBufferCount;

            auto& nextResource = impl->m_frameResources[impl->m_frameResourceIndex];

            if (nextResource.needFence && impl->m_fence->GetCompletedValue() < nextResource.fenceValue)
            {
                const auto event = CreateEvent(nullptr, false, false, nullptr);
                impl->m_fence->SetEventOnCompletion(nextResource.fenceValue, event);
                WaitForSingleObjectEx(event, INFINITE, false);
                CloseHandle(event);
            }

            nextResource.needFence = true;

            // コマンドアロケータのリセット
            nextResource.commandAllocator->Reset();

            // コマンドリストのリセット
            nextResource.commandList->Reset(nextResource.commandAllocator.Get(), nullptr);
        }
    }
};

namespace TY::detail
{
    CommandList::CommandList(CommandListType type) :
        p_impl(std::make_shared<Impl>(type))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    void CommandList::CloseAndFlush()
    {
        if (p_impl) p_impl->CloseAndFlush();
    }

    void CommandList::SequenceCloseAndFlush(const Array<CommandList>& list)
    {
        Impl::SequenceCloseAndFlush(list);
    }

    ID3D12GraphicsCommandList* CommandList::GetCommandList() const
    {
        return p_impl ? p_impl->m_frameResources[p_impl->m_frameResourceIndex].commandList.Get() : nullptr;
    }

    ID3D12CommandQueue* CommandList::GetCommandQueue() const
    {
        return p_impl ? p_impl->m_commandQueue.Get() : nullptr;
    }
}
