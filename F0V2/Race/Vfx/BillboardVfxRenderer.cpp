#include "pch.h"
#include "BillboardVfxRenderer.h"

#include "Asset.generated.h"
#include "TY/DynamicBinding.h"
#include "TY/GenericModelBufferTemplates.h"
#include "TY/GenericModelDrawer.h"

using namespace Race;

namespace
{
    struct GpuParticleElement
    {
        Float3 worldPosition;
        float rotation;
        Float2 size;
        Float2 padding;
        Float4 color;
    };

    struct BillboardParticle_b10
    {
        Float3 cameraUp;
        float padding0;
        Float3 cameraRight;
        float padding1;
    };

    static_assert(sizeof(GpuParticleElement) == 48);
    static_assert(sizeof(BillboardParticle_b10) == 32);
}

struct BillboardVfxRenderer::Impl
{
    int m_capacity{};
    IndexBuffer m_indexBuffer{Empty};
    GenericModelDrawer m_drawer{};
    DynamicSrvHandle m_particleSrv{};
    BillboardParticle_b10 m_particleCB{};

    Impl(const ImagePathWrapper& image, int capacity, GraphicsBlendOptions blendOptions) :
        m_capacity(capacity)
    {
        assert(not image.isEmpty());
        assert(capacity > 0);

        m_indexBuffer = IndexBuffer::Placeholder(0);
        const auto model = std::make_shared<SingleShapeModelBuffer>(m_indexBuffer);

        m_drawer = GenericModelDrawer{
            GenericModelDrawerParams{}
            .setModel(model)
            .setVertexInput({})
            .setOptions(
                GraphicsOptions()
                .setBlend(blendOptions)
                .setDepth(
                    GraphicsDepthOptions()
                    .setTestEnabled(true)
                    .setWriteMask(false)))
            .setShader(Asset_shader::billboard_effect)
            .setDynamicCbvCount(1)
            .setSrv10AndLater({image.fetchResource()})
            .setDynamicSrvCount(1)
        };
    }

    void Upload(
        const Array<BillboardVfxElement>& elements,
        const Float3& cameraUp,
        const Float3& cameraRight)
    {
        assert(elements.size() <= m_capacity);

        const int uploadCount = Min(static_cast<int>(elements.size()), m_capacity);
        Array<GpuParticleElement> gpuElements{};
        gpuElements.reserve(uploadCount);

        for (int i = 0; i < uploadCount; ++i)
        {
            const auto& element = elements[i];
            gpuElements.push_back(GpuParticleElement{
                .worldPosition = element.worldPosition,
                .rotation = element.rotation,
                .size = element.size,
                .color = element.color.toFloat4(),
            });
        }

        m_particleCB = BillboardParticle_b10{
            .cameraUp = cameraUp,
            .cameraRight = cameraRight,
        };

        m_particleSrv = DynamicSrvHandle{};
        if (not gpuElements.empty())
        {
            m_particleSrv = DynamicBinding::UploadDynamicStructuredBuffer(
                std::span<const GpuParticleElement>{gpuElements.data(), gpuElements.size()});
        }

        m_indexBuffer.resize(uploadCount * 6);
    }
};

namespace Race
{
    void BillboardVfxRenderer::init(const ImagePathWrapper& image, int capacity, GraphicsBlendOptions blendOptions)
    {
        assert(not p_impl);
        p_impl = std::make_shared<Impl>(image, capacity, blendOptions);
    }

    void BillboardVfxRenderer::finalize()
    {
        p_impl.reset();
    }

    int BillboardVfxRenderer::capacity() const
    {
        return p_impl ? p_impl->m_capacity : 0;
    }

    void BillboardVfxRenderer::upload(
        const Array<BillboardVfxElement>& elements,
        const Float3& cameraUp,
        const Float3& cameraRight)
    {
        assert(p_impl);
        if (p_impl)
        {
            p_impl->Upload(elements, cameraUp, cameraRight);
        }
    }

    void BillboardVfxRenderer::draw() const
    {
        if (p_impl && p_impl->m_particleSrv.address != 0)
        {
            DynamicBinding::SetDynamicSrv(11, p_impl->m_particleSrv);
            DynamicBinding::SetDynamicCbv(10, p_impl->m_particleCB);
            p_impl->m_drawer.draw();
        }
    }
}
