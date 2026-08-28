#include "pch.h"
#include "DynamicBinding.h"

#include "detail/ComponentManager_singleton.h"
#include "detail/RenderContext_singleton.h"
#include "IComponent.h"
#include "Logger.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr size_t UploadPageSize = 64 * 1024;
    constexpr size_t ConstantBufferAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    constexpr size_t VertexBufferAlignment = 1;
    constexpr size_t IndexBufferAlignment = sizeof(uint16_t);

    struct DynamicBufferAllocation
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

        std::optional<DynamicBufferAllocation> allocate(size_t size, size_t alignment)
        {
            if (size == 0 || alignment == 0 || m_offset > m_capacity)
            {
                return std::nullopt;
            }

            const size_t remainingSize = m_capacity - m_offset;
            const size_t remainder = m_offset % alignment;
            const size_t padding = remainder == 0 ? 0 : alignment - remainder;
            if (padding > remainingSize || size > remainingSize - padding)
            {
                return std::nullopt;
            }

            const size_t offset = m_offset + padding;
            m_offset = offset + size;

            return DynamicBufferAllocation{
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

        std::optional<DynamicBufferAllocation> allocate(size_t size, size_t alignment)
        {
            while (pageIndex < pages.size())
            {
                if (auto allocation = pages[pageIndex]->allocate(size, alignment))
                {
                    return allocation;
                }

                ++pageIndex;
            }

            const size_t capacity = std::max(UploadPageSize, size);
            auto page = std::make_unique<UploadPage>(capacity);
            if (not page->isValid())
            {
                return std::nullopt;
            }

            auto allocation = page->allocate(size, alignment);
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

        D3D12_GPU_VIRTUAL_ADDRESS upload(const void* data, size_t size, size_t alignment)
        {
            if (not data || size == 0)
            {
                return 0;
            }

            const size_t timestamp = RenderContext_singleton::GetFlushTimestamp();
            auto& frameResource = m_frameResources[timestamp % RenderContext_singleton::FrameBufferCount];
            frameResource.beginFrame(timestamp);

            const auto allocation = frameResource.allocate(size, alignment);
            if (not allocation)
            {
                return 0;
            }

            std::memcpy(allocation->cpuAddress, data, size);
            return allocation->gpuAddress;
        }

        void setCbvByAddress(RootParameterIndex rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
        {
            m_pendingBindings[rootParameterIndex.value] = address;
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
            if (not s_instance)
            {
                LogError("DynamicBindingComponent: Instance is not initialized.");
            }

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
    DynamicVertexBufferHandle UploadDynamicVertexBuffer(const void* data, size_t size)
    {
        const auto component = DynamicBindingComponent::instance();
        assert(component);
        return DynamicVertexBufferHandle(component ? component->upload(data, size, VertexBufferAlignment) : 0);
    }

    DynamicIndexBufferHandle UploadDynamicIndexBuffer(const void* data, size_t size)
    {
        const auto component = DynamicBindingComponent::instance();
        assert(component);
        return DynamicIndexBufferHandle(component ? component->upload(data, size, IndexBufferAlignment) : 0);
    }

    DynamicCbvHandle UploadDynamicCbv(const void* data, size_t size)
    {
        const auto component = DynamicBindingComponent::instance();
        assert(component);
        return DynamicCbvHandle(component ? component->upload(data, size, ConstantBufferAlignment) : 0);
    }

    void SetDynamicCbv(RootParameterIndex rootParameterIndex, const void* data, size_t size)
    {
        const auto component = DynamicBindingComponent::instance();
        assert(component);

        const auto address = component->upload(data, size, ConstantBufferAlignment);
        if (address != 0)
        {
            component->setCbvByAddress(rootParameterIndex, address);
        }
    }

    void SetDynamicCbv(RootParameterIndex rootParameterIndex, DynamicCbvHandle cbv)
    {
        if (rootParameterIndex.value < 0 || cbv.address == 0)
        {
            LogError("DynamicBinding::SetDynamicCbvByAddress: Invalid argument.");
            assert(false);
            return;
        }

        const auto component = DynamicBindingComponent::instance();
        if (not component)
        {
            LogError("DynamicBinding::SetDynamicCbvByAddress: DynamicBindingComponent is not initialized.");
            assert(false);
            return;
        }

        component->setCbvByAddress(rootParameterIndex, cbv.address);
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
