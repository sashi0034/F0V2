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
    ComPtr<ID3D12CommandAllocator> m_commandAllocator{};
    ComPtr<ID3D12GraphicsCommandList> m_commandList{};
    ComPtr<ID3D12CommandQueue> m_commandQueue{};

    ComPtr<ID3D12Fence> m_fence{};
    UINT64 m_fenceValue{};

    Impl(CommandListType type)
    {
        const auto device = EngineRenderContext::GetDevice();
        const auto commandListType = getCommandListType(type);

        // コマンドアロケータを生成
        if (const HRESULT hr = device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&m_commandAllocator));
            FAILED(hr))
        {
            LogError(std::format("CreateCommandAllocator failed: {}", hr));
            return;
        }

        m_commandAllocator->SetName(L"CommandAllocator");

        // コマンドリストを生成
        if (const HRESULT hr = device->CreateCommandList(
                0,
                commandListType,
                m_commandAllocator.Get(),
                nullptr,
                IID_PPV_ARGS(&m_commandList));
            FAILED(hr))
        {
            LogError(std::format("CreateCommandList failed: {}", hr));
            return;
        }

        m_commandList->SetName(L"CommandList");

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
        m_commandList->Close();

        ID3D12CommandList* commandLists[] = {m_commandList.Get()};
        m_commandQueue->ExecuteCommandLists(1, commandLists);

        // 実行の待機
        m_fenceValue++;
        m_commandQueue->Signal(m_fence.Get(), m_fenceValue);

        if (m_fence->GetCompletedValue() != m_fenceValue)
        {
            const auto event = CreateEvent(nullptr, false, false, nullptr);
            m_fence->SetEventOnCompletion(m_fenceValue, event);
            WaitForSingleObjectEx(event, INFINITE, false);
            CloseHandle(event);
        }

        // コマンドアロケータのリセット
        m_commandAllocator->Reset();

        // コマンドリストのリセット
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);
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

    ID3D12GraphicsCommandList* CommandList::GetCommandList() const
    {
        return p_impl ? p_impl->m_commandList.Get() : nullptr;
    }

    ID3D12CommandQueue* CommandList::GetCommandQueue() const
    {
        return p_impl ? p_impl->m_commandQueue.Get() : nullptr;
    }
}
