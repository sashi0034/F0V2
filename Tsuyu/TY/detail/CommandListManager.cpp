#include "pch.h"
#include "CommandListManager.h"

#include "RenderContext_singleton.h"
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
        case CommandListType::Draw:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
            // case CommandListType::Copy:
            //     return D3D12_COMMAND_LIST_TYPE_COPY;
            // case CommandListType::Compute:
            //     return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        }

        assert(false);
        return {};
    }
}

struct CommandListManager::Impl
{
    bool m_valid{};

    struct frame_resource
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator{};
        ComPtr<ID3D12GraphicsCommandList> commandList{};

        UINT64 fenceValue{};
        bool needFence{};

        ComPtr<ID3D12QueryHeap> timestampQuery{};
        ComPtr<ID3D12Resource> timestampBuffer{};

        UINT64* mappedTimestamps{}; // [0]: start, [1]: end
        float lastExecutionMilliseconds{};

        bool isFirstExecution() const
        {
            return not needFence;
        }
    };

    Array<frame_resource> m_frameResources{RenderContext_singleton::FrameBufferCount};

    uint8_t m_frameResourceIndex{0};

    ComPtr<ID3D12CommandQueue> m_commandQueue{};

    ComPtr<ID3D12Fence> m_fence{};

    Impl(CommandListType type)
    {
        const auto device = RenderContext_singleton::GetDevice();
        const auto commandListType = getCommandListType(type);

        for (int i = 0; i < RenderContext_singleton::FrameBufferCount; ++i)
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

            // -----------------------------------------------

            D3D12_QUERY_HEAP_DESC heapDesc{};
            heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            heapDesc.Count = 2; // start / end
            device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&rsc.timestampQuery));

            D3D12_RESOURCE_DESC bufDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT64) * 2);
            auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
            if (const HRESULT hr = device->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &bufDesc,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_PPV_ARGS(&rsc.timestampBuffer));
                FAILED(hr))
            {
                LogError("CreateCommittedResource failed: {}", hr);
                return;
            }
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

    ~Impl()
    {
        for (auto& frameResource : m_frameResources)
        {
            if (frameResource.mappedTimestamps)
            {
                frameResource.timestampBuffer->Unmap(0, nullptr);
                frameResource.mappedTimestamps = nullptr;
            }
        }
    }

    // void CloseAndFlush(const Impl* lastCommandList)
    void CloseAndAdvance()
    {
        auto& currentResource = m_frameResources[m_frameResourceIndex];

        // 打刻終了
        if (not currentResource.isFirstExecution())
        {
            currentResource.commandList->EndQuery(currentResource.timestampQuery.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
            currentResource.commandList->ResolveQueryData(
                currentResource.timestampQuery.Get(),
                D3D12_QUERY_TYPE_TIMESTAMP,
                0,
                2,
                currentResource.timestampBuffer.Get(),
                0);
        }

        // コマンドリスト終了
        currentResource.commandList->Close();

        ID3D12CommandList* commandLists[] = {currentResource.commandList.Get()};
        m_commandQueue->ExecuteCommandLists(1, commandLists);

        // 実行の待機
        currentResource.fenceValue += RenderContext_singleton::FrameBufferCount;
        m_commandQueue->Signal(m_fence.Get(), currentResource.fenceValue);

        // current
        // -----------------------------------------------
        // next

        m_frameResourceIndex = (m_frameResourceIndex + 1) % RenderContext_singleton::FrameBufferCount;

        auto& nextResource = m_frameResources[m_frameResourceIndex];

        if (nextResource.needFence && m_fence->GetCompletedValue() < nextResource.fenceValue)
        {
            const auto event = CreateEvent(nullptr, false, false, nullptr);
            m_fence->SetEventOnCompletion(nextResource.fenceValue, event);
            WaitForSingleObjectEx(event, INFINITE, false);
            CloseHandle(event);
        }

        nextResource.needFence = true;

        // プロファイリング結果の取得
        {
            if (not nextResource.mappedTimestamps)
            {
                nextResource.timestampBuffer->Map(0, nullptr, reinterpret_cast<void**>(&nextResource.mappedTimestamps));
            }

            if (nextResource.mappedTimestamps)
            {
                UINT64 start = nextResource.mappedTimestamps[0];
                UINT64 end = nextResource.mappedTimestamps[1];

                UINT64 freq{};
                m_commandQueue->GetTimestampFrequency(&freq);

                nextResource.lastExecutionMilliseconds =
                    1000.0f * static_cast<float>(end - start) / static_cast<float>(freq);
            }
        }

        // コマンドアロケータのリセット
        nextResource.commandAllocator->Reset();

        // コマンドリストのリセット
        nextResource.commandList->Reset(nextResource.commandAllocator.Get(), nullptr);

        // 打刻開始
        nextResource.commandList->EndQuery(nextResource.timestampQuery.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    }

    void WaitLastFlush()
    {
        auto& previousResource = m_frameResources[previousFrameResourceIndex()];

        if (previousResource.needFence && m_fence->GetCompletedValue() < previousResource.fenceValue)
        {
            const auto event = CreateEvent(nullptr, false, false, nullptr);
            m_fence->SetEventOnCompletion(previousResource.fenceValue, event);
            WaitForSingleObjectEx(event, INFINITE, false);
            CloseHandle(event);
        }
    }

private:
    uint8_t previousFrameResourceIndex() const
    {
        constexpr int frameBufferCount = RenderContext_singleton::FrameBufferCount;
        return (m_frameResourceIndex - 1 + frameBufferCount) % frameBufferCount;
    }
};

namespace TY::detail
{
    CommandListManager::CommandListManager(CommandListType type) :
        p_impl(std::make_shared<Impl>(type))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    void CommandListManager::closeAndAdvance()
    {
        if (p_impl) p_impl->CloseAndAdvance();
    }

    // void CommandList::CloseAndFlushAfter(const CommandList& lastCommandList)
    // {
    //     if (p_impl) p_impl->CloseAndFlush(lastCommandList.p_impl.get());
    // }

    void CommandListManager::waitLastCommandList()
    {
        if (p_impl) { p_impl->WaitLastFlush(); }
    }

    ID3D12GraphicsCommandList* CommandListManager::getCommandList() const
    {
        return p_impl ? p_impl->m_frameResources[p_impl->m_frameResourceIndex].commandList.Get() : nullptr;
    }

    ID3D12CommandQueue* CommandListManager::getCommandQueue() const
    {
        return p_impl ? p_impl->m_commandQueue.Get() : nullptr;
    }

    float CommandListManager::lastExecutionMilliseconds() const
    {
        return p_impl ? p_impl->m_frameResources[p_impl->m_frameResourceIndex].lastExecutionMilliseconds : 0.0;
    }
}
