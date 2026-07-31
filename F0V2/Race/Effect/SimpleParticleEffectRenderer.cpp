#include "pch.h"
#include "SimpleParticleEffectRenderer.h"

#include "Asset.generated.h"
#include "TY/ConstantBufferArray.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/GenericModelBuffer.h"
#include "TY/GenericModelDrawer.h"
#include "TY/StructuredBufferWrapper.h"

using namespace Race;

namespace
{
    struct DrawerModelBuffer : IGenericModelBuffer
    {
        GenericModelShapeBufferElement m_shape{};

        DrawerModelBuffer()
        {
            m_shape.materialIndex = 0;
            m_shape.indexBuffer = IndexBuffer::Placeholder(0);
        }

        int shapeCount() const override
        {
            return 1;
        }

        GenericModelShapeBufferElement shapeAt(int index) const override
        {
            assert(index == 0);
            return m_shape;
        }

        int materialCount() const override
        {
            return 1;
        }

        ConstantBufferArrayImpl materialCbv() const override
        {
            return {Empty};
        }

        Array<Array<ShaderResourceType>> materialSrv() const override
        {
            return {};
        }
    };

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

struct SimpleParticleEffectRenderer::Impl
{
    int m_capacity{};
    IndexBuffer m_indexBuffer{Empty};
    GenericModelDrawer m_drawer{};
    StructuredBufferT<GpuParticleElement> m_particleBuffer{};
    ConstantBufferWrapper<SimpleParticle_b10> m_particleCB{};

    Impl(const ImagePathWrapper& image, int capacity) :
        m_capacity(capacity),
        m_particleBuffer(capacity)
    {
        assert(not image.isEmpty());
        assert(capacity > 0);

        const auto model = std::make_shared<DrawerModelBuffer>();
        m_indexBuffer = model->m_shape.indexBuffer;

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
            .setCbv10AndLater({m_particleCB})
            .setSrv10AndLater({image.fetchResource(), m_particleBuffer})
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

        m_particleCB.uploadValue(SimpleParticle_b10{
            .cameraUp = cameraUp,
            .cameraRight = cameraRight,
        });

        if (not gpuElements.empty())
        {
            m_particleBuffer.upload(gpuElements);
        }

        m_indexBuffer.resize(uploadCount * 6);
    }
};

namespace Race
{
    void SimpleParticleEffectRenderer::init(const ImagePathWrapper& image, int capacity)
    {
        assert(not p_impl);
        p_impl = std::make_shared<Impl>(image, capacity);
    }

    void SimpleParticleEffectRenderer::finalize()
    {
        p_impl.reset();
    }

    void SimpleParticleEffectRenderer::upload(
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

    void SimpleParticleEffectRenderer::draw() const
    {
        if (p_impl)
        {
            p_impl->m_drawer.draw();
        }
    }
}
