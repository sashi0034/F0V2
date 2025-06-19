#include "pch.h"
#include "Gpgpu.h"

#include "ConstantBuffer.h"
#include "Logger.h"
#include "detail/ComputePipelineState.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr int maxBufferCount = 8;

    struct BufferInfo_b0
    {
        std::array<std::array<uint32_t, 4>, maxBufferCount> writableBufferSize{};
        std::array<std::array<uint32_t, 4>, maxBufferCount> readonlyBufferSize{};
    };
}

struct Gpgpu::Impl
{
    GpgpuParams m_params{};

    ConstantBuffer<BufferInfo_b0> m_cb0{};

    Array<StructuredBufferUploader> m_sr{};
    Array<StructuredBufferTransfer> m_ua{};

    ComputePipelineState m_computePipelineState{};

    DescriptorHeap m_descriptorHeap{};

    Impl(const GpgpuParams& params) : m_params(params)
    {
        if (params.readonlyBuffer.size() > maxBufferCount || params.writableBuffer.size() > maxBufferCount)
        {
            LogError.writeln("Gpgpu: Too many buffers specified. Maximum is " + std::to_string(maxBufferCount));
            return;
        }

        m_sr.resize(params.readonlyBuffer.size());
        for (int i = 0; i < params.readonlyBuffer.size(); ++i)
        {
            m_sr[i] = StructuredBufferUploader({
                .elementCount = params.readonlyBuffer[i]->getElementCount(),
                .elementStride = params.readonlyBuffer[i]->getElementStride()
            });
        }

        m_ua.resize(params.writableBuffer.size());
        for (int i = 0; i < params.writableBuffer.size(); ++i)
        {
            m_ua[i] = StructuredBufferTransfer({
                .elementCount = params.writableBuffer[i]->getElementCount(),
                .elementStride = params.writableBuffer[i]->getElementStride()
            });
        }

        const auto descriptorTable = DescriptorTable{DescriptorTableElement{2, m_sr.size(), m_ua.size()}};
        m_computePipelineState = ComputePipelineState({
            .computeShader = params.cs,
            .descriptorTable = descriptorTable
        });

        m_descriptorHeap = DescriptorHeap(DescriptorHeapParams{
            .table = m_computePipelineState.descriptorTable(),
            .materialCounts = {1},
            .descriptors = {
                CbSrUaSet{
                    {m_cb0, params.cb1},
                    m_sr.toColumnVector<ShaderResourceType>(),
                    m_ua.toColumnVector<StructuredBufferUploader>()
                }
            }
        });

        for (int i = 0; i < params.readonlyBuffer.size(); ++i)
        {
            m_cb0->readonlyBufferSize[i][0] = static_cast<uint32_t>(m_params.readonlyBuffer[i]->getElementCount());
        }

        for (int i = 0; i < params.writableBuffer.size(); ++i)
        {
            m_cb0->writableBufferSize[i][0] = static_cast<uint32_t>(m_params.writableBuffer[i]->getElementCount());
        }

        m_cb0.upload();
    }

    void Compute()
    {
        const auto commandTargetLifetime = EngineRenderContext::ScopedCommandTarget(CommandListType::Compute);
        for (int i = 0; i < m_params.readonlyBuffer.size(); ++i)
        {
            m_sr[i].upload(m_params.readonlyBuffer[i]->getDataPointer());
        }

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            m_ua[i].upload(m_params.writableBuffer[i]->getDataPointer());
        }

        m_computePipelineState.commandSet();
        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetTable(PipelineType::Compute, 0);

        const auto commandList = EngineRenderContext::ActiveCommandList();
        const auto mainUA = m_ua[0];

        const auto mainSize3D = m_params.writableBuffer[0]->getSize3D();
        Integer3D<UINT> threadGroup{1, 1, 1};;
        if (mainSize3D.y <= 1 && mainSize3D.z <= 1)
        {
            // 1D Buffer
            static constexpr double groutCount = 64.0;
            threadGroup.x = static_cast<UINT>(ceil(mainSize3D.x / groutCount));
        }
        else if (mainSize3D.x <= 1)
        {
            // 2D Buffer
            static constexpr double groutCount = 8.0;
            threadGroup.x = static_cast<UINT>(ceil(mainSize3D.x / groutCount));
            threadGroup.y = static_cast<UINT>(ceil(mainSize3D.y / groutCount));
        }
        else
        {
            // 3D Buffer
            static constexpr double groutCount = 4.0;
            threadGroup.x = static_cast<UINT>(ceil(mainSize3D.x / groutCount));
            threadGroup.y = static_cast<UINT>(ceil(mainSize3D.y / groutCount));
            threadGroup.z = static_cast<UINT>(ceil(mainSize3D.z / groutCount));
        }

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
};

namespace TY
{
    Gpgpu::Gpgpu(const GpgpuParams& params)
        : p_impl(std::make_shared<Impl>(params))
    {
    }

    void Gpgpu::compute()
    {
        if (p_impl) p_impl->Compute();
    }
}
