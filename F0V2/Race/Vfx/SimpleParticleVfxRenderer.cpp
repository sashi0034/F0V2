#include "pch.h"
#include "SimpleParticleVfxRenderer.h"

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
        Float3 rgb;
        float alpha;
        float scale;
    };

    struct SimpleParticle_b10
    {
        Float3 cameraUp;
        float padding0;
        Float3 cameraRight;
        float padding1;
    };

    static_assert(sizeof(GpuParticleElement) == 32);
    static_assert(sizeof(SimpleParticle_b10) == 32);
}

struct SimpleParticleVfxRenderer::Impl
{
    int m_capacity{};
    IndexBuffer m_indexBuffer{Empty};
    GenericModelDrawer m_drawer{};
    DynamicSrvHandle m_particleSrv{};
    SimpleParticle_b10 m_particleCB{};

    Impl(const ImagePathWrapper& image, int capacity) :
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
                .setBlend(GraphicsBlendOptions::AlphaBlend())
                .setDepth(
                    GraphicsDepthOptions()
                    .setTestEnabled(true)
                    .setWriteMask(false)))
            .setShader(Asset_shader::simple_particle)
            .setDynamicCbvCount(1)
            .setSrv10AndLater({image.fetchResource()})
            .setDynamicSrvCount(1)
        };
    }

    void Upload(
        const Array<SimpleParticleRenderElement>& elements,
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
                .rgb = element.color.toFloat3(),
                .alpha = element.color.a,
                .scale = element.scale,
            });
        }

        m_particleCB = SimpleParticle_b10{
            .cameraUp = cameraUp,
            .cameraRight = cameraRight,
        };

        m_particleSrv = DynamicSrvHandle{};
        if (not gpuElements.empty())
        {
            m_particleSrv = DynamicBinding::UploadDynamicStructuredBuffer(
                std::span{gpuElements.data(), gpuElements.size()});
        }

        m_indexBuffer.resize(uploadCount * 6);
    }
};

namespace Race
{
    void SimpleParticleVfxRenderer::init(const ImagePathWrapper& image, int capacity)
    {
        assert(not p_impl);
        p_impl = std::make_shared<Impl>(image, capacity);
    }

    void SimpleParticleVfxRenderer::finalize()
    {
        p_impl.reset();
    }

    void SimpleParticleVfxRenderer::upload(
        const Array<SimpleParticleRenderElement>& elements,
        const Float3& cameraUp,
        const Float3& cameraRight)
    {
        assert(p_impl);
        if (p_impl)
        {
            p_impl->Upload(elements, cameraUp, cameraRight);
        }
    }

    void SimpleParticleVfxRenderer::draw() const
    {
        if (p_impl && p_impl->m_particleSrv.address != 0)
        {
            DynamicBinding::SetDynamicCbv(10, p_impl->m_particleCB);

            DynamicBinding::SetDynamicSrv(11, p_impl->m_particleSrv);

            p_impl->m_drawer.draw();
        }
    }
}
