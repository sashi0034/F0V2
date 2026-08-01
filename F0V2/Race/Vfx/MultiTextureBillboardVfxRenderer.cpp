#include "pch.h"
#include "MultiTextureBillboardVfxRenderer.h"

using namespace Race;

struct MultiTextureBillboardVfxRenderer::Impl
{
    int m_capacity{};
    Array<BillboardVfxRenderer> m_renderers{};

    Impl(
        const Array<ImagePathWrapper>& images,
        int capacity,
        const GraphicsBlendOptions& blendOptions) :
        m_capacity(capacity)
    {
        m_renderers.reserve(images.size());
        for (const auto& image : images)
        {
            BillboardVfxRenderer renderer{};
            renderer.init(image, capacity, blendOptions);
            m_renderers.push_back(std::move(renderer));
        }
    }

    void Upload(
        const Array<BillboardVfxElement>& elements,
        const Float3& cameraUp,
        const Float3& cameraRight)
    {
        assert(elements.size() <= m_capacity);

        const int textureCount = static_cast<int>(m_renderers.size());
        Array<Array<BillboardVfxElement>> elementsPerTexture(textureCount);
        for (const auto& element : elements)
        {
            const bool textureIndexIsValid =
                element.textureIndex_ >= 0 && element.textureIndex_ < textureCount;
            assert(textureIndexIsValid);
            if (not textureIndexIsValid)
            {
                continue;
            }

            elementsPerTexture[element.textureIndex_].push_back(element);
        }

        for (int i = 0; i < textureCount; ++i)
        {
            m_renderers[i].upload(elementsPerTexture[i], cameraUp, cameraRight);
        }
    }

    void Finalize()
    {
        for (auto& renderer : m_renderers)
        {
            renderer.finalize();
        }
        m_renderers.clear();
    }

    void Draw() const
    {
        for (const auto& renderer : m_renderers)
        {
            renderer.draw();
        }
    }
};

namespace Race
{
    void MultiTextureBillboardVfxRenderer::init(
        const Array<ImagePathWrapper>& images,
        int capacity,
        GraphicsBlendOptions blendOptions)
    {
        assert(not p_impl);
        assert(not images.empty());
        assert(capacity > 0);
        if (p_impl || images.empty() || capacity <= 0)
        {
            return;
        }

        p_impl = std::make_shared<Impl>(images, capacity, blendOptions);
    }

    void MultiTextureBillboardVfxRenderer::finalize()
    {
        if (p_impl)
        {
            p_impl->Finalize();
            p_impl.reset();
        }
    }

    int MultiTextureBillboardVfxRenderer::capacity() const
    {
        return p_impl ? p_impl->m_capacity : 0;
    }

    int MultiTextureBillboardVfxRenderer::textureCount() const
    {
        return p_impl ? static_cast<int>(p_impl->m_renderers.size()) : 0;
    }

    void MultiTextureBillboardVfxRenderer::upload(
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

    void MultiTextureBillboardVfxRenderer::draw() const
    {
        if (p_impl)
        {
            p_impl->Draw();
        }
    }
}
