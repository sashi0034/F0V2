#include "pch.h"
#include "MultiTextureQuadVfxRenderer.h"

using namespace Race;

struct MultiTextureQuadVfxRenderer::Impl
{
    int m_capacity{};
    Array<QuadVfxRenderer> m_renderers{};

    Impl(
        const Array<ImagePathWrapper>& images,
        int capacity,
        const GraphicsBlendOptions& blendOptions) :
        m_capacity(capacity)
    {
        m_renderers.reserve(images.size());
        for (const auto& image : images)
        {
            QuadVfxRenderer renderer{};
            renderer.init(image, capacity, blendOptions);
            m_renderers.push_back(std::move(renderer));
        }
    }

    void Upload(const Array<QuadVfxElement>& elements)
    {
        assert(elements.size() <= m_capacity);

        const int textureCount = static_cast<int>(m_renderers.size());
        Array<Array<QuadVfxElement>> elementsPerTexture(textureCount);
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
            m_renderers[i].upload(elementsPerTexture[i]);
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
    void MultiTextureQuadVfxRenderer::init(
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

    void MultiTextureQuadVfxRenderer::finalize()
    {
        if (p_impl)
        {
            p_impl->Finalize();
            p_impl.reset();
        }
    }

    int MultiTextureQuadVfxRenderer::capacity() const
    {
        return p_impl ? p_impl->m_capacity : 0;
    }

    int MultiTextureQuadVfxRenderer::textureCount() const
    {
        return p_impl ? static_cast<int>(p_impl->m_renderers.size()) : 0;
    }

    void MultiTextureQuadVfxRenderer::upload(const Array<QuadVfxElement>& elements)
    {
        assert(p_impl);
        if (p_impl)
        {
            p_impl->Upload(elements);
        }
    }

    void MultiTextureQuadVfxRenderer::draw() const
    {
        if (p_impl)
        {
            p_impl->Draw();
        }
    }
}
