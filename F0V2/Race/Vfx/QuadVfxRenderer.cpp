#include "pch.h"
#include "QuadVfxRenderer.h"

#include "Asset.generated.h"
#include "TY/DynamicBinding.h"
#include "TY/GenericModelBufferTemplates.h"
#include "TY/GenericModelDrawer.h"

using namespace Race;

namespace
{
    struct GpuQuadElement
    {
        Float3 worldPosition;
        float padding;
        Float3 right;
        float sizeX;
        Float3 up;
        float sizeY;
        Float4 color;
    };

    static_assert(sizeof(GpuQuadElement) == 64);
}

struct QuadVfxRenderer::Impl
{
    int m_capacity{};
    IndexBuffer m_indexBuffer{Empty};
    GenericModelDrawer m_drawer{};
    DynamicSrvHandle m_quadSrv{};

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
            .setShader(Asset_shader::quad_vfx)
            .setSrv10AndLater({image.fetchResource()})
            .setDynamicSrvCount(1)
        };
    }

    void Upload(const Array<QuadVfxElement>& elements)
    {
        assert(elements.size() <= m_capacity);

        const int uploadCount = Min(static_cast<int>(elements.size()), m_capacity);
        Array<GpuQuadElement> gpuElements{};
        gpuElements.reserve(uploadCount);

        for (int i = 0; i < uploadCount; ++i)
        {
            const auto& element = elements[i];
            gpuElements.push_back(GpuQuadElement{
                .worldPosition = element.worldPosition,
                .right = element.rotation.rotate(Float3{1.0f, 0.0f, 0.0f}),
                .sizeX = element.size.x,
                .up = element.rotation.rotate(Float3{0.0f, 1.0f, 0.0f}),
                .sizeY = element.size.y,
                .color = element.color.toFloat4(),
            });
        }

        m_quadSrv = DynamicSrvHandle{};
        if (not gpuElements.empty())
        {
            m_quadSrv = DynamicBinding::UploadDynamicStructuredBuffer(
                std::span<const GpuQuadElement>{gpuElements.data(), gpuElements.size()});
        }

        m_indexBuffer.resize(uploadCount * 6);
    }
};

namespace Race
{
    void QuadVfxRenderer::init(
        const ImagePathWrapper& image,
        int capacity,
        GraphicsBlendOptions blendOptions)
    {
        assert(not p_impl);
        p_impl = std::make_shared<Impl>(image, capacity, blendOptions);
    }

    void QuadVfxRenderer::finalize()
    {
        p_impl.reset();
    }

    int QuadVfxRenderer::capacity() const
    {
        return p_impl ? p_impl->m_capacity : 0;
    }

    void QuadVfxRenderer::upload(const Array<QuadVfxElement>& elements)
    {
        assert(p_impl);
        if (p_impl)
        {
            p_impl->Upload(elements);
        }
    }

    void QuadVfxRenderer::draw() const
    {
        if (p_impl && p_impl->m_quadSrv.address != 0)
        {
            DynamicBinding::SetDynamicSrv(11, p_impl->m_quadSrv);
            p_impl->m_drawer.draw();
        }
    }
}
