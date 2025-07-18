#include "pch.h"
#include "Gpgpu.h"

#include "ConstantBuffer.h"
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

    struct StructuredBufferUploaderCache
    {
        std::weak_ptr<IGpgpuBuffer> lifetime;
        StructuredBufferUploader structuredBuffer;
    };

    struct StructuredBufferTransferCache
    {
        std::weak_ptr<IGpgpuBuffer> lifetime;
        StructuredBufferTransfer structuredBuffer;
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
            cleanUp(m_structuredBufferUploaderCache);
            cleanUp(m_structuredBufferTransferCache);
            return true;
        }

        StructuredBufferUploader FetchStructuredBufferUploader(
            const std::shared_ptr<IGpgpuBuffer>& key)
        {
            if (not key)
            {
                return {};
            }

            auto& cache = m_structuredBufferUploaderCache;
            if (const auto it = cache.find(key.get()); it != cache.end())
            {
                return it->second.structuredBuffer;
            }

            const auto& cache2 = m_structuredBufferTransferCache;
            if (const auto it2 = cache2.find(key.get()); it2 != cache2.end())
            {
                return it2->second.structuredBuffer;
            }

            StructuredBufferUploader uploader{StructuredBufferTransferParams::From(key)};
            cache[key.get()] = {key, uploader};
            return uploader;
        }

        StructuredBufferTransfer FetchStructuredBufferTransfer(
            const std::shared_ptr<IGpgpuBuffer>& key)
        {
            if (not key)
            {
                return {};
            }

            auto& cache = m_structuredBufferTransferCache;

            if (const auto it = cache.find(key.get()); it != cache.end())
            {
                return it->second.structuredBuffer;
            }

            StructuredBufferTransfer transfer{StructuredBufferTransferParams::From(key)};
            cache[key.get()] = {key, transfer};
            return transfer;
        }

    private:
        std::unordered_map<IGpgpuBuffer*, StructuredBufferUploaderCache> m_structuredBufferUploaderCache{};

        std::unordered_map<IGpgpuBuffer*, StructuredBufferTransferCache> m_structuredBufferTransferCache{};

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

    ConstantBufferUploaderCore m_cb0{Empty};
    ConstantBufferUploaderCore m_cb1{Empty};

    Array<StructuredBufferUploader> m_sr{};
    Array<StructuredBufferTransfer> m_ua{};

    ComputePipelineState m_computePipelineState{};

    DescriptorHeap m_descriptorHeap{};

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
        m_cb0 = ConstantBufferUploaderCore(sizeof(uint32_t) * 4 * m_params.readonlyBuffer.size());
        m_cb1 = ConstantBufferUploaderCore(sizeof(uint32_t) * 4 * m_params.writableBuffer.size());

        m_sr.resize(m_params.readonlyBuffer.size());
        for (int i = 0; i < m_params.readonlyBuffer.size(); ++i)
        {
            if (access(m_params.readonlyBuffer[i]).getElementCount() > 0)
            {
                m_sr[i] = getCache().FetchStructuredBufferUploader(m_params.readonlyBuffer[i]);
            }
        }

        m_ua.resize(m_params.writableBuffer.size());
        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            if (access(m_params.writableBuffer[i]).getElementCount() > 0)
            {
                m_ua[i] = getCache().FetchStructuredBufferTransfer(m_params.writableBuffer[i]);
            }
        }

        const auto descriptorTable = DescriptorTable{DescriptorTableElement{3, m_sr.size(), m_ua.size()}};
        m_computePipelineState = ComputePipelineState({
            .computeShader = m_params.cs,
            .descriptorTable = descriptorTable
        });

        m_descriptorHeap = DescriptorHeap(DescriptorHeapParams{
            .table = m_computePipelineState.descriptorTable(),
            .materialCounts = {1},
            .descriptors = {
                CbvSrvUavSet{
                    {m_cb0, m_cb1, m_params.cb2},
                    m_sr.toColumnVector<ShaderResourceType>(),
                    m_ua.toColumnVector<UnorderedAccessType>()
                }
            }
        });
    }

    void Compute()
    {
        checkResized();

        const auto commandTargetLifetime = EngineRenderContext::ScopedCommandTarget(CommandListType::Compute);
        for (int i = 0; i < m_params.readonlyBuffer.size(); ++i)
        {
            m_sr[i].upload(access(m_params.readonlyBuffer[i]).readonlyDataPointer());
        }

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            m_ua[i].upload(access(m_params.writableBuffer[i]).readonlyDataPointer());
        }

        m_computePipelineState.commandSet();
        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetTable(PipelineType::Compute, 0);

        const auto commandList = EngineRenderContext::ActiveCommandList();
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

        EngineRenderContext::FlushActiveCommandList();

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

        const auto commandTargetLifetime = EngineRenderContext::ScopedCommandTarget(CommandListType::Compute);

        std::unordered_map<IReadonlyGpgpu*, StructuredBufferUploader> srMap{};
        std::unordered_map<IReadonlyGpgpu*, StructuredBufferTransfer> uaMap{};

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
            impl->m_computePipelineState.commandSet();
            impl->m_descriptorHeap.commandSet();
            impl->m_descriptorHeap.commandSetTable(PipelineType::Compute, 0);

            const auto commandList = EngineRenderContext::ActiveCommandList();
            const auto mainUA = impl->m_ua[0];

            const auto mainSize3D = access(impl->m_params.writableBuffer[0]).getSize3D();

            const Integer3D<UINT> threadGroup = getThreadGroup(mainSize3D);
            commandList->Dispatch(threadGroup.x, threadGroup.y, threadGroup.z);

            // for (int i = 0; i < impl->m_params.writableBuffer.size(); ++i)
            // {
            //     impl->m_ua[i].afterDispatch();
            // }
            StructuredBufferTransfer::AfterDispatch(impl->m_ua);
        }

        // for (auto& ua : uaMap)
        // {
        //     ua.second.beforeFlush();
        // }
        {
            Array<StructuredBufferTransfer> transfers{uaMap.size()};
            for (const auto& ua : uaMap)
            {
                transfers.push_back(ua.second);
            }

            StructuredBufferTransfer::BeforeFlush(transfers);
        }

        EngineRenderContext::FlushActiveCommandList();

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
                m_sr[i] = getCache().FetchStructuredBufferTransfer(m_params.readonlyBuffer[i]);
                m_descriptorHeap.resetSrv(m_sr[i], 0, i);

                resized = true;
            }
        }

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            if (m_ua[i].elementCount() != access(m_params.writableBuffer[i]).getElementCount())
            {
                m_ua[i] = getCache().FetchStructuredBufferTransfer(m_params.writableBuffer[i]);
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
