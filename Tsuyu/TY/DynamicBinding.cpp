#include "pch.h"
#include "DynamicBinding.h"

#include "detail/ComponentManager_singleton.h"
#include "detail/RenderContext_singleton.h"
#include "IComponent.h"
#include "Logger.h"
#include "Utils.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr size_t UploadPageSize = 64 * 1024;
    constexpr size_t ConstantBufferAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

    struct DynamicConstantBufferAllocation
    {
        uint8_t* cpuAddress{};
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{};
    };

    class UploadPage
    {
    public:
        explicit UploadPage(size_t capacity)
            : m_capacity(capacity)
        {
            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(capacity);

            if (const HRESULT hr = RenderContext_singleton::GetDevice()->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(m_resource.ReleaseAndGetAddressOf()));
                FAILED(hr))
            {
                LogError(std::format("DynamicBinding: Failed to create upload page: {}", static_cast<int>(hr)));
                return;
            }

            m_resource->SetName(L"DynamicBinding::UploadPage");

            if (const HRESULT hr = m_resource->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedAddress)); FAILED(hr))
            {
                LogError(std::format("DynamicBinding: Failed to map upload page: {}", static_cast<int>(hr)));
                return;
            }

            m_valid = true;
        }

        ~UploadPage()
        {
            if (m_resource && m_mappedAddress)
            {
                m_resource->Unmap(0, nullptr);
                m_mappedAddress = nullptr;
            }

            RenderContext_singleton::SafeDisposeRenderResource(m_resource);
        }

        bool isValid() const
        {
            return m_valid;
        }

        void reset()
        {
            m_offset = 0;
        }

        std::optional<DynamicConstantBufferAllocation> allocate(size_t size)
        {
            const size_t alignedSize = AlignedSize(size, ConstantBufferAlignment);
            if (alignedSize > m_capacity - m_offset)
            {
                return std::nullopt;
            }

            const size_t offset = m_offset;
            m_offset += alignedSize;

            return DynamicConstantBufferAllocation{
                .cpuAddress = m_mappedAddress + offset,
                .gpuAddress = m_resource->GetGPUVirtualAddress() + offset,
            };
        }

    private:
        bool m_valid{};
        size_t m_capacity{};
        size_t m_offset{};
        ComPtr<ID3D12Resource> m_resource{};
        uint8_t* m_mappedAddress{};
    };

    struct FrameResource
    {
        size_t timestamp{std::numeric_limits<size_t>::max()};
        size_t pageIndex{};
        Array<std::unique_ptr<UploadPage>> pages{};

        void beginFrame(size_t currentTimestamp)
        {
            if (timestamp == currentTimestamp)
            {
                return;
            }

            timestamp = currentTimestamp;
            pageIndex = 0;
            for (const auto& page : pages)
            {
                page->reset();
            }
        }

        std::optional<DynamicConstantBufferAllocation> allocate(size_t size)
        {
            while (pageIndex < pages.size())
            {
                if (auto allocation = pages[pageIndex]->allocate(size))
                {
                    return allocation;
                }

                ++pageIndex;
            }

            const size_t capacity = std::max(UploadPageSize, AlignedSize(size, ConstantBufferAlignment));
            auto page = std::make_unique<UploadPage>(capacity);
            if (not page->isValid())
            {
                return std::nullopt;
            }

            auto allocation = page->allocate(size);
            pages.push_back(std::move(page));
            return allocation;
        }
    };

    class DynamicBindingComponent final : public IComponent
    {
    public:
        bool init() override
        {
            if (s_instance)
            {
                assert(false);
                return false;
            }

            s_instance = this;
            return true;
        }

        ~DynamicBindingComponent() override
        {
            if (s_instance == this)
            {
                s_instance = nullptr;
            }
        }

        void afterPresent() override
        {
            m_pendingBindings.clear();
        }

        void setCbv(RootParameterIndex rootParameterIndex, const void* data, size_t size)
        {
            const size_t timestamp = RenderContext_singleton::GetFlushTimestamp();
            auto& frameResource = m_frameResources[timestamp % RenderContext_singleton::FrameBufferCount];
            frameResource.beginFrame(timestamp);

            const auto allocation = frameResource.allocate(size);
            if (not allocation)
            {
                return;
            }

            std::memcpy(allocation->cpuAddress, data, size);
            m_pendingBindings[rootParameterIndex.value] = allocation->gpuAddress;
        }

        void commandSetGraphicsCbv()
        {
            const auto commandList = RenderContext_singleton::TargetCommandList();
            for (const auto& [rootParameterIndex, gpuAddress] : m_pendingBindings)
            {
                commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAddress);
            }

            m_pendingBindings.clear();
        }

        static DynamicBindingComponent* instance()
        {
            return s_instance;
        }

    private:
        static inline DynamicBindingComponent* s_instance{};

        std::array<FrameResource, RenderContext_singleton::FrameBufferCount> m_frameResources{};
        std::unordered_map<int, D3D12_GPU_VIRTUAL_ADDRESS> m_pendingBindings{};
    };
}

namespace TY::DynamicBinding
{
    void SetDynamicCbv(RootParameterIndex rootParameterIndex, const void* data, size_t size)
    {
        if (rootParameterIndex.value < 0 || not data || size == 0)
        {
            LogError("DynamicBinding::SetDynamicCbv: Invalid argument.");
            assert(false);
            return;
        }

        const auto component = DynamicBindingComponent::instance();
        if (not component)
        {
            LogError("DynamicBinding::SetDynamicCbv: DynamicBindingComponent is not initialized.");
            assert(false);
            return;
        }

        component->setCbv(rootParameterIndex, data, size);
    }

    void FlushAsGraphics()
    {
        if (const auto component = DynamicBindingComponent::instance())
        {
            component->commandSetGraphicsCbv();
        }
    }
}

namespace TY::detail
{
    void InitDynamicBindingComponent()
    {
        ComponentManager_singleton::Register<DynamicBindingComponent>("DynamicBindingComponent");
    }
}
