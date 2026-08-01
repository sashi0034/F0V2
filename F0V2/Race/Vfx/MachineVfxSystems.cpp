#include "pch.h"
#include "MachineVfxSystems.h"

#include "Asset.generated.h"
#include "BillboardVfxRenderer.h"
#include "TY/Array.h"

using namespace Race;

namespace
{
    constexpr int driftSparkCapacity = 2048;
    constexpr int collisionRingCapacity = 512;

    struct DriftSparkParticle
    {
        Float3 worldPosition{};
        Float3 velocity{};
        float rotation{};
        float angularVelocity{8.0f};
        float age{};
        float lifetime{0.25f};
    };

    struct CollisionRingParticle
    {
        Float3 worldPosition{};
        float age{};
        float lifetime{0.3f};
        ColorF32 startColor{};
        ColorF32 endColor{};
    };
}

// TODO: 修正
struct DriftSparkVfxSystem::Impl
{
    BillboardVfxRenderer m_renderer{};
    Array<DriftSparkParticle> m_particles{};

    void Emit(const DriftSparkVfxSpawnParams& params)
    {
        if (m_particles.size() >= driftSparkCapacity)
        {
            return;
        }

        m_particles.push_back(DriftSparkParticle{
            .worldPosition = params.worldPosition,
            .velocity = params.velocity,
        });
    }

    void Update(const RaceVfxFrameContext& context)
    {
        const ColorF32 startColor{1.0f, 0.75f, 0.2f, 1.0f};
        const ColorF32 endColor{1.0f, 0.3f, 0.05f, 0.0f};

        Array<BillboardVfxRenderElement> renderElements{};
        renderElements.reserve(m_particles.size());

        for (int i = static_cast<int>(m_particles.size()) - 1; i >= 0; --i)
        {
            auto& particle = m_particles[i];
            particle.age += context.deltaTime;
            particle.worldPosition += particle.velocity * context.deltaTime;
            particle.rotation += particle.angularVelocity * context.deltaTime;
            if (particle.age >= particle.lifetime)
            {
                m_particles.remove_at(i);
                continue;
            }

            const float rate = Math::Clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
            const float scale = std::lerp(0.45f, 0.05f, rate);
            renderElements.push_back(BillboardVfxRenderElement{
                .worldPosition = particle.worldPosition,
                .rotation = particle.rotation,
                .size = Float2{scale, scale},
                .color = startColor.lerp(endColor, rate),
            });
        }

        m_renderer.upload(renderElements, context.cameraUp, context.cameraRight);
    }
};

// TODO: 修正
struct CollisionRingVfxSystem::Impl
{
    BillboardVfxRenderer m_renderer{};
    Array<CollisionRingParticle> m_particles{};

    void Emit(const CollisionRingVfxSpawnParams& params)
    {
        if (m_particles.size() >= collisionRingCapacity)
        {
            return;
        }

        ColorF32 endColor = params.color;
        endColor.a = 0.0f;

        m_particles.push_back(CollisionRingParticle{
            .worldPosition = params.worldPosition,
            .startColor = params.color,
            .endColor = endColor,
        });
    }

    void Update(const RaceVfxFrameContext& context)
    {
        Array<BillboardVfxRenderElement> renderElements{};
        renderElements.reserve(m_particles.size());

        for (int i = static_cast<int>(m_particles.size()) - 1; i >= 0; --i)
        {
            auto& particle = m_particles[i];
            particle.age += context.deltaTime;
            if (particle.age >= particle.lifetime)
            {
                m_particles.remove_at(i);
                continue;
            }

            const float rate = Math::Clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
            const float scale = std::lerp(1.0f, 4.0f, rate);
            renderElements.push_back(BillboardVfxRenderElement{
                .worldPosition = particle.worldPosition,
                .size = Float2{scale, scale},
                .color = particle.startColor.lerp(particle.endColor, rate),
            });
        }

        m_renderer.upload(renderElements, context.cameraUp, context.cameraRight);
    }
};

namespace Race
{
    DriftSparkVfxSystem::DriftSparkVfxSystem() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void DriftSparkVfxSystem::emit(const DriftSparkVfxSpawnParams& params)
    {
        p_impl->Emit(params);
    }

    void DriftSparkVfxSystem::onRegistered()
    {
        p_impl->m_renderer.init(Asset_image::spark_01, driftSparkCapacity);
    }

    void DriftSparkVfxSystem::update(const RaceVfxFrameContext& context)
    {
        p_impl->Update(context);
    }

    void DriftSparkVfxSystem::drawTransparent() const
    {
        p_impl->m_renderer.draw();
    }

    void DriftSparkVfxSystem::onUnregistered()
    {
        p_impl->m_particles.clear();
        p_impl->m_renderer.finalize();
    }

    CollisionRingVfxSystem::CollisionRingVfxSystem() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void CollisionRingVfxSystem::emit(const CollisionRingVfxSpawnParams& params)
    {
        p_impl->Emit(params);
    }

    void CollisionRingVfxSystem::onRegistered()
    {
        p_impl->m_renderer.init(Asset_image::flame_01, collisionRingCapacity);
    }

    void CollisionRingVfxSystem::update(const RaceVfxFrameContext& context)
    {
        p_impl->Update(context);
    }

    void CollisionRingVfxSystem::drawTransparent() const
    {
        p_impl->m_renderer.draw();
    }

    void CollisionRingVfxSystem::onUnregistered()
    {
        p_impl->m_particles.clear();
        p_impl->m_renderer.finalize();
    }
}
