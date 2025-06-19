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
        std::array<uint32_t, 4> t0_size{};
        std::array<uint32_t, 4> u0_size{};
    };

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
}

struct Gpgpu::Impl
{
    bool m_valid{};

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

        Setup();

        uploadCB0(m_params);

        m_valid = true;
    }

    void Setup()
    {
        m_sr.resize(m_params.readonlyBuffer.size());
        for (int i = 0; i < m_params.readonlyBuffer.size(); ++i)
        {
            if (access(m_params.readonlyBuffer[i]).getElementCount() > 0)
            {
                m_sr[i] = StructuredBufferUploader(StructuredBufferTransferParams::From(m_params.readonlyBuffer[i]));
            }
        }

        m_ua.resize(m_params.writableBuffer.size());
        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            if (access(m_params.writableBuffer[i]).getElementCount() > 0)
            {
                m_ua[i] = StructuredBufferTransfer(StructuredBufferTransferParams::From(m_params.writableBuffer[i]));
            }
        }

        const auto descriptorTable = DescriptorTable{DescriptorTableElement{2, m_sr.size(), m_ua.size()}};
        m_computePipelineState = ComputePipelineState({
            .computeShader = m_params.cs,
            .descriptorTable = descriptorTable
        });

        m_descriptorHeap = DescriptorHeap(DescriptorHeapParams{
            .table = m_computePipelineState.descriptorTable(),
            .materialCounts = {1},
            .descriptors = {
                CbSrUaSet{
                    {m_cb0, m_params.cb1},
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

private:
    void uploadCB0(const GpgpuParams& params)
    {
        const auto t0_size = params.readonlyBuffer.empty() ? Point3D{} : params.readonlyBuffer[0]->getSize3D();
        m_cb0->t0_size[0] = static_cast<uint32_t>(t0_size.x);
        m_cb0->t0_size[1] = static_cast<uint32_t>(t0_size.y);
        m_cb0->t0_size[2] = static_cast<uint32_t>(t0_size.z);

        const auto u0_size = params.writableBuffer.empty() ? Point3D{} : params.writableBuffer[0]->getSize3D();
        m_cb0->u0_size[0] = static_cast<uint32_t>(u0_size.x);
        m_cb0->u0_size[1] = static_cast<uint32_t>(u0_size.y);
        m_cb0->u0_size[2] = static_cast<uint32_t>(u0_size.z);

        m_cb0.upload();
    }

    void checkResized()
    {
        bool resized{};

        for (int i = 0; i < m_params.readonlyBuffer.size(); ++i)
        {
            if (m_sr[i].elementCount() != access(m_params.readonlyBuffer[i]).getElementCount())
            {
                m_sr[i] =
                    access(m_params.readonlyBuffer[i]).getElementCount() > 0
                        ? StructuredBufferUploader(StructuredBufferTransferParams::From(m_params.readonlyBuffer[i]))
                        : StructuredBufferUploader{};
                m_descriptorHeap.resetSRV(m_sr[i], 0, i);

                resized = true;
            }
        }

        for (int i = 0; i < m_params.writableBuffer.size(); ++i)
        {
            if (m_ua[i].elementCount() != access(m_params.writableBuffer[i]).getElementCount())
            {
                m_ua[i] =
                    access(m_params.writableBuffer[i]).getElementCount() > 0
                        ? StructuredBufferTransfer(StructuredBufferTransferParams::From(m_params.writableBuffer[i]))
                        : StructuredBufferTransfer{};
                m_descriptorHeap.resetUAV(m_ua[i], 0, i);

                resized = true;
            }
        }

        if (resized)
        {
            uploadCB0(m_params);
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
}
