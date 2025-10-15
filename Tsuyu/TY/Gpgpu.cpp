#include "pch.h"
#include "Gpgpu.h"

#include "ConstantBufferWrapper.h"
#include "IComponent.h"
#include "Logger.h"
#include "detail/ComputePipelineState.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineComponent.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr int readonlyBufferCapacity = 128;

    constexpr int writableBufferCapacity = 64;

    struct EmptyGpgpuBuffer : IGpgpuBuffer
    {
        const void* readonlyDataPointer() override { return nullptr; };

        void* writableDataPointer() override { return nullptr; };

        int getElementCount() const override { return 0; };

        int getElementStride() const override { return 0; };

        Point3D getSize3D() const override { return Point3D{}; };
    };

    IGpgpuBuffer& access(const std::shared_ptr<IGpgpuBuffer>& buffer)
    {
        if (buffer)
        {
            return *buffer;
        }
        else
        {
            static EmptyGpgpuBuffer empty{};
            return empty;
        }
    }

    Integer3D<UINT> getThreadGroup(const Point3D targetSize)
    {
        Integer3D<UINT> threadGroup = {1, 1, 1};;
        if (targetSize.y <= 1 && targetSize.z <= 1)
        {
            // 1D Buffer
            static constexpr double groutCount = 64.0;
            threadGroup.x = static_cast<UINT>(ceil(targetSize.x / groutCount));
        }
        else if (targetSize.z <= 1)
        {
            // 2D Buffer
            static constexpr double groutCount = 8.0;
            threadGroup.x = static_cast<UINT>(ceil(targetSize.x / groutCount));
            threadGroup.y = static_cast<UINT>(ceil(targetSize.y / groutCount));
        }
        else
        {
            // 3D Buffer
            static constexpr double groutCount = 4.0;
            threadGroup.x = static_cast<UINT>(ceil(targetSize.x / groutCount));
            threadGroup.y = static_cast<UINT>(ceil(targetSize.y / groutCount));
            threadGroup.z = static_cast<UINT>(ceil(targetSize.z / groutCount));
        }

        return threadGroup;
    }

    struct StructuredBufferCache
    {
        std::weak_ptr<IGpgpuBuffer> lifetime;
        StructuredBuffer structuredBuffer;
    };

    struct UnorderedStructuredBufferCache
    {
        std::weak_ptr<IGpgpuBuffer> lifetime;
        UnorderedStructuredBuffer structuredBuffer;
    };

    struct GpgpuCacheComponent;

    GpgpuCacheComponent* s_cache{};

    GpgpuCacheComponent& getCache()
    {
        assert(s_cache);
        return *s_cache;
    }

    struct GpgpuCacheComponent : IComponent
    {
        bool init() override
        {
            if (s_cache)
            {
                assert(false);
                return false;
            }

            s_cache = this;
            return true;
        }

        ~GpgpuCacheComponent()
        {
            if (s_cache == this)
            {
                s_cache = nullptr;
            }
        }

        bool update() override
        {
            cleanUp(m_StructuredBufferCache);
            cleanUp(m_UnorderedStructuredBufferCache);
            return true;
        }

        StructuredBuffer FetchStructuredBuffer(
            const std::shared_ptr<IGpgpuBuffer>& key)
        {
            if (not key)
            {
                return {};
            }

            auto& cache = m_StructuredBufferCache;
            if (const auto it = cache.find(key.get()); it != cache.end())
            {
                return it->second.structuredBuffer;
            }

            const auto& cache2 = m_UnorderedStructuredBufferCache;
            if (const auto it2 = cache2.find(key.get()); it2 != cache2.end())
            {
                return it2->second.structuredBuffer;
            }

            StructuredBuffer uploader{UnorderedStructuredBufferParams::From(key)};
            cache[key.get()] = {key, uploader};
            return uploader;
        }

        UnorderedStructuredBuffer FetchUnorderedStructuredBuffer(
            const std::shared_ptr<IGpgpuBuffer>& key)
        {
            if (not key)
            {
                return {};
            }

            auto& cache = m_UnorderedStructuredBufferCache;

            if (const auto it = cache.find(key.get()); it != cache.end())
            {
                return it->second.structuredBuffer;
            }

            UnorderedStructuredBuffer transfer{UnorderedStructuredBufferParams::From(key)};
            cache[key.get()] = {key, transfer};
            return transfer;
        }

    private:
        std::unordered_map<IGpgpuBuffer*, StructuredBufferCache> m_StructuredBufferCache{};

        std::unordered_map<IGpgpuBuffer*, UnorderedStructuredBufferCache> m_UnorderedStructuredBufferCache{};

        template <typename Container>
        void cleanUp(Container& cache)
        {
            for (auto it = cache.begin(); it != cache.end();)
            {
                if (it->second.lifetime.expired())
                {
                    it = cache.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    };
}

struct Gpgpu::Impl
{
    bool m_valid{};

    GpgpuParams m_params{};

    ConstantBufferCore m_cb0{Empty};
    ConstantBufferCore m_cb1{Empty};

    Array<StructuredBuffer> m_sr{};
    Array<UnorderedStructuredBuffer> m_ua{};

    ComputePipelineState m_computePipelineState{};

    DescriptorHeap m_descriptorHeap{};

    int m_tableIndexofCbv10AndLater{-1};

    Impl(const GpgpuParams& params) : m_params(params)
    {
        if (params.readonlyBuffer.size() > readonlyBufferCapacity)
        {
            LogError.writeln(std::format(
                "Gpgpu: Too many readonly buffers specified. Maximum is {}",
                readonlyBufferCapacity));
            return;
        }

        if (params.writableBuffer.size() > writableBufferCapacity)
        {
            LogError.writeln(std::format(
                "Gpgpu: Too many writable buffers specified. Maximum is {}",
                writableBufferCapacity));
            return;
        }

        Setup();

        uploadCB0_1(m_params);

        m_valid = true;
    }

    void Setup()
    {
        m_cb0 = ConstantBufferCore(sizeof(uint32_t) * 4 * m_params.readonlyBuffer.size());
        m_cb1 = ConstantBufferCore(sizeof(uint32_t) * 4 * m_params.writableBuffer.size());

        m_sr.resize(m_params.readonlyBuffer.size());
        for (int i = 0; i < m_params.readonlyBuffer.size(); ++i)
        {
            if (access(m_params.readonlyBuffer[i]).getElementCount() > 0)
            {
                m_sr[i] = getCache().FetchStructuredBuffer(m_params.readonlyBuffer[i]);
            }
        }

        m_ua.resize(m_params.writableBuffer.size());
        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            if (access(m_params.writableBuffer[i]).getElementCount() > 0)
            {
                m_ua[i] = getCache().FetchUnorderedStructuredBuffer(m_params.writableBuffer[i]);
            }
        }

        auto descriptorHeap = DescriptorHeapParams{
            .table = DescriptorTable{
                DescriptorTableElement{2, m_sr.size(), m_ua.size()},
            },
            .materialCounts = {1},
            .descriptors = {
                CbvSrvUavSet{
                    {m_cb0, m_cb1},
                    m_sr.toColumnVector<ShaderResourceType>(),
                    m_ua.toColumnVector<UnorderedAccessType>()
                },

            }
        };

        if (m_params.cbv10AndLater.size() > 0)
        {
            m_tableIndexofCbv10AndLater = static_cast<int>(descriptorHeap.table.size());

            descriptorHeap.table.push_back(DescriptorTableElement{m_params.cbv10AndLater.size(), 0, 0});
            descriptorHeap.materialCounts.push_back(1);
            descriptorHeap.descriptors.push_back(CbvSrvUavSet{
                m_params.cbv10AndLater,
                {},
                {}
            });
        }

        m_computePipelineState = ComputePipelineState(ComputePipelineStateParams{
            .computeShader = m_params.cs,
            .descriptorTable = descriptorHeap.table,
            .explicitRegisterStarts = {ShaderRegisterStart{1, 10}}
        });

        m_descriptorHeap = DescriptorHeap(descriptorHeap);
    }

    void Compute()
    {
        checkResized();

        for (int i = 0; i < m_params.readonlyBuffer.size(); ++i)
        {
            m_sr[i].upload(access(m_params.readonlyBuffer[i]).readonlyDataPointer());
        }

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            m_ua[i].upload(access(m_params.writableBuffer[i]).readonlyDataPointer());
        }

        m_computePipelineState.commandSet(CommandListType::Compute);
        m_descriptorHeap.commandSet(PipelineType::Compute);
        m_descriptorHeap.commandSetComputeTable(CommandListType::Compute, 0);

        if (m_tableIndexofCbv10AndLater != -1)
        {
            m_descriptorHeap.commandSetComputeTable(CommandListType::Compute, m_tableIndexofCbv10AndLater);
        }

        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Compute);
        const auto mainUA = m_ua[0];

        const auto mainSize3D = access(m_params.writableBuffer[0]).getSize3D();

        const Integer3D<UINT> threadGroup = getThreadGroup(mainSize3D);
        commandList->Dispatch(threadGroup.x, threadGroup.y, threadGroup.z);

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            m_ua[i].afterDispatch();
        }

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            m_ua[i].beforeFlush();
        }

        EngineRenderContext::FlushComputeCommandSync();

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            m_ua[i].readback(m_params.writableBuffer[i]->writableDataPointer());
        }
    }

    static void SequenceCompute(const Array<std::shared_ptr<Impl>>& list)
    {
        for (auto& impl : list)
        {
            impl->checkResized();
        }

        std::unordered_map<IReadonlyGpgpu*, StructuredBuffer> srMap{};
        std::unordered_map<IReadonlyGpgpu*, UnorderedStructuredBuffer> uaMap{};

        for (auto& impl : list)
        {
            for (int i = 0; i < impl->m_params.writableBuffer.size(); ++i)
            {
                if (uaMap.contains(impl->m_params.writableBuffer[i].get()))
                {
                    continue;
                }

                impl->m_ua[i].upload(access(impl->m_params.writableBuffer[i]).readonlyDataPointer());
                uaMap[impl->m_params.writableBuffer[i].get()] = impl->m_ua[i];
            }
        }

        for (auto& impl : list)
        {
            for (int i = 0; i < impl->m_params.readonlyBuffer.size(); ++i)
            {
                if (uaMap.contains(impl->m_params.readonlyBuffer[i].get()))
                {
                    // バッファを UA として転送済みなら SR としては転送しない
                    continue;
                }

                if (srMap.contains(impl->m_params.readonlyBuffer[i].get()))
                {
                    continue;
                }

                impl->m_sr[i].upload(access(impl->m_params.readonlyBuffer[i]).readonlyDataPointer());
                srMap[impl->m_params.readonlyBuffer[i].get()] = impl->m_sr[i];
            }
        }

        for (auto& impl : list)
        {
            impl->m_computePipelineState.commandSet(CommandListType::Compute);
            impl->m_descriptorHeap.commandSet(PipelineType::Compute);
            impl->m_descriptorHeap.commandSetComputeTable(CommandListType::Compute, 0);

            if (impl->m_tableIndexofCbv10AndLater != -1)
            {
                impl->m_descriptorHeap.commandSetComputeTable(CommandListType::Compute, impl->m_tableIndexofCbv10AndLater);
            }

            const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Compute);
            const auto mainUA = impl->m_ua[0];

            const auto mainSize3D = access(impl->m_params.writableBuffer[0]).getSize3D();

            const Integer3D<UINT> threadGroup = getThreadGroup(mainSize3D);
            commandList->Dispatch(threadGroup.x, threadGroup.y, threadGroup.z);

            // for (int i = 0; i < impl->m_params.writableBuffer.size(); ++i)
            // {
            //     impl->m_ua[i].afterDispatch();
            // }
            UnorderedStructuredBuffer::AfterDispatch(impl->m_ua);
        }

        // for (auto& ua : uaMap)
        // {
        //     ua.second.beforeFlush();
        // }
        {
            Array<UnorderedStructuredBuffer> transfers{uaMap.size()};
            for (const auto& ua : uaMap)
            {
                transfers.push_back(ua.second);
            }

            UnorderedStructuredBuffer::BeforeFlush(transfers);
        }

        EngineRenderContext::FlushComputeCommandSync();

        for (auto& ua : uaMap)
        {
            ua.second.readback(ua.first->writableDataPointer());
        }
    }

private:
    void uploadCB0_1(const GpgpuParams& params)
    {
        Array<std::array<uint32_t, 4>> sr_sizes{};
        sr_sizes.reserve(params.readonlyBuffer.size());
        for (int i = 0; i < params.readonlyBuffer.size(); ++i)
        {
            const auto& buffer = access(params.readonlyBuffer[i]);
            sr_sizes.push_back({
                static_cast<uint32_t>(buffer.getSize3D().x),
                static_cast<uint32_t>(buffer.getSize3D().y),
                static_cast<uint32_t>(buffer.getSize3D().z),
                0
            });
        }

        Array<std::array<uint32_t, 4>> ua_sizes{};
        ua_sizes.reserve(params.writableBuffer.size());
        for (int i = 0; i < params.writableBuffer.size(); ++i)
        {
            const auto& buffer = access(params.writableBuffer[i]);
            ua_sizes.push_back({
                static_cast<uint32_t>(buffer.getSize3D().x),
                static_cast<uint32_t>(buffer.getSize3D().y),
                static_cast<uint32_t>(buffer.getSize3D().z),
                0
            });
        }

        assert(sr_sizes.size_in_bytes() == m_cb0.sizeInBytes());
        m_cb0.upload(sr_sizes.data());

        assert(ua_sizes.size_in_bytes() == m_cb1.sizeInBytes());
        m_cb1.upload(ua_sizes.data());
    }

    void checkResized()
    {
        bool resized{};

        for (int i = 0; i < m_params.readonlyBuffer.size(); ++i)
        {
            if (m_sr[i].elementCount() != access(m_params.readonlyBuffer[i]).getElementCount())
            {
                m_sr[i] = getCache().FetchUnorderedStructuredBuffer(m_params.readonlyBuffer[i]);
                m_descriptorHeap.resetSrv(m_sr[i], 0, i);

                resized = true;
            }
        }

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            if (m_ua[i].elementCount() != access(m_params.writableBuffer[i]).getElementCount())
            {
                m_ua[i] = getCache().FetchUnorderedStructuredBuffer(m_params.writableBuffer[i]);
                m_descriptorHeap.resetUav(m_ua[i], 0, i);

                resized = true;
            }
        }

        if (resized)
        {
            uploadCB0_1(m_params);
        }
    }
};

namespace TY
{
    Gpgpu::Gpgpu(const GpgpuParams& params)
        : p_impl(std::make_shared<Impl>(params))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    void Gpgpu::compute()
    {
        if (p_impl) p_impl->Compute();
    }

    void Gpgpu::SequenceCompute(const Array<Gpgpu>& list)
    {
        Impl::SequenceCompute(
            list.filter([](const Gpgpu& gpgpu) -> bool { return gpgpu.p_impl != nullptr; })
                .map([](const Gpgpu& gpgpu) { return gpgpu.p_impl; })
        );
    }

    namespace detail
    {
        void InitGpgpuCacheComponent()
        {
            EngineComponent::Register<GpgpuCacheComponent>("GpgpuCacheComponent");
        }
    }
}
