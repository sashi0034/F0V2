#include "pch.h"
#include "Gpgpu.h"

#include "ConstantBuffer.h"
#include "Logger.h"
#include "detail/ComputePipelineState.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineCacheContext.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr int readonlyBufferCapacity = 128;

    constexpr int writableBufferCapacity = 64;

    struct EmptyGpgpuBuffer : IGpgpuBuffer
    {
        void* getDataPointer() override { return nullptr; };

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
        else if (targetSize.x <= 1)
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
}

struct Gpgpu::Impl
{
    bool m_valid{};

    GpgpuParams m_params{};

    ConstantBufferUploader_impl m_cb0{Empty};
    ConstantBufferUploader_impl m_cb1{Empty};

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
        m_cb0 = ConstantBufferUploader_impl(sizeof(uint32_t) * 4 * m_params.readonlyBuffer.size());
        m_cb1 = ConstantBufferUploader_impl(sizeof(uint32_t) * 4 * m_params.writableBuffer.size());

        m_sr.resize(m_params.readonlyBuffer.size());
        for (int i = 0; i < m_params.readonlyBuffer.size(); ++i)
        {
            if (access(m_params.readonlyBuffer[i]).getElementCount() > 0)
            {
                m_sr[i] = EngineCacheContext::FetchStructuredBufferUploader(m_params.readonlyBuffer[i]);
            }
        }

        m_ua.resize(m_params.writableBuffer.size());
        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            if (access(m_params.writableBuffer[i]).getElementCount() > 0)
            {
                m_ua[i] = EngineCacheContext::FetchStructuredBufferTransfer(m_params.writableBuffer[i]);
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
                CbSrUaSet{
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
            m_sr[i].upload(access(m_params.readonlyBuffer[i]).getDataPointer());
        }

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            m_ua[i].upload(access(m_params.writableBuffer[i]).getDataPointer());
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
            m_ua[i].readback(m_params.writableBuffer[i]->getDataPointer());
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
            for (int i = 0; i < impl->m_params.readonlyBuffer.size(); ++i)
            {
                if (srMap.contains(impl->m_params.readonlyBuffer[i].get())) continue;

                impl->m_sr[i].upload(access(impl->m_params.readonlyBuffer[i]).getDataPointer());
                srMap[impl->m_params.readonlyBuffer[i].get()] = impl->m_sr[i];
            }

            for (int i = 0; i < impl->m_params.writableBuffer.size(); ++i)
            {
                if (uaMap.contains(impl->m_params.writableBuffer[i].get())) continue;

                impl->m_ua[i].upload(access(impl->m_params.writableBuffer[i]).getDataPointer());
                uaMap[impl->m_params.writableBuffer[i].get()] = impl->m_ua[i];
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

            for (int i = 0; i < impl->m_params.writableBuffer.size(); ++i)
            {
                impl->m_ua[i].afterDispatch();
            }
        }

        for (auto& ua : uaMap)
        {
            ua.second.beforeFlush();
        }

        EngineRenderContext::FlushActiveCommandList();

        for (auto& ua : uaMap)
        {
            ua.second.readback(ua.first->getDataPointer());
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
                m_sr[i] = EngineCacheContext::FetchStructuredBufferTransfer(m_params.readonlyBuffer[i]);
                m_descriptorHeap.resetSRV(m_sr[i], 0, i);

                resized = true;
            }
        }

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            if (m_ua[i].elementCount() != access(m_params.writableBuffer[i]).getElementCount())
            {
                m_ua[i] = EngineCacheContext::FetchStructuredBufferTransfer(m_params.writableBuffer[i]);
                m_descriptorHeap.resetUAV(m_ua[i], 0, i);

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
}
