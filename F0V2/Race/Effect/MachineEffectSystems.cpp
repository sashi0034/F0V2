#include "pch.h"
#include "MachineEffectSystems.h"

#include "Asset.generated.h"
#include "BillboardEffectRenderer.h"
#include "SimpleParticleEffectRenderer.h"
#include "TY/Array.h"

using namespace Race;

namespace
{
    constexpr int boostTrailCapacity = 4096;
    constexpr int driftSparkCapacity = 2048;
    constexpr int collisionRingCapacity = 512;

    struct BoostTrailParticle
    {
        Float3 worldPosition{};
        ColorF32 color{};
        float scale{};
    };

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

struct BoostTrailEffectSystem::Impl
{
    SimpleParticleEffectRenderer m_renderer{};
    Array<BoostTrailParticle> m_particles{};

    void Emit(const BoostTrailEffectSpawnParams& params)
    {
        if (m_particles.size() >= boostTrailCapacity)
        {
            return;
        }

        ColorF32 color = params.color;
        color.a = 1.0f;
        m_particles.push_back(BoostTrailParticle{
            .worldPosition = params.worldPosition,
            .color = color,
            .scale = 1.0f + params.intensity,
        });
    }

    void Update(const RaceEffectFrameContext& context)
    {
        Array<SimpleParticleRenderElement> renderElements{};
        renderElements.reserve(m_particles.size());

        for (int i = static_cast<int>(m_particles.size()) - 1; i >= 0; --i)
        {
            auto& particle = m_particles[i];
            particle.color.a -= context.deltaTime;
            particle.scale = Max(0.0f, particle.scale - 5.0f * context.deltaTime);
            if (particle.color.a <= 0.0f)
            {
                m_particles.remove_at(i);
                continue;
            }

            renderElements.push_back(SimpleParticleRenderElement{
                .worldPosition = particle.worldPosition,
                .color = particle.color,
                .scale = particle.scale,
            });
        }

        m_renderer.upload(renderElements, context.cameraUp, context.cameraRight);
    }
};

// TODO: 修正
struct DriftSparkEffectSystem::Impl
{
    BillboardEffectRenderer m_renderer{};
    Array<DriftSparkParticle> m_particles{};

    void Emit(const DriftSparkEffectSpawnParams& params)
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

    void Update(const RaceEffectFrameContext& context)
    {
        const ColorF32 startColor{1.0f, 0.75f, 0.2f, 1.0f};
        const ColorF32 endColor{1.0f, 0.3f, 0.05f, 0.0f};

        Array<BillboardEffectRenderElement> renderElements{};
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
            renderElements.push_back(BillboardEffectRenderElement{
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
struct CollisionRingEffectSystem::Impl
{
    BillboardEffectRenderer m_renderer{};
    Array<CollisionRingParticle> m_particles{};

    void Emit(const CollisionRingEffectSpawnParams& params)
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

    void Update(const RaceEffectFrameContext& context)
    {
        Array<BillboardEffectRenderElement> renderElements{};
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
            renderElements.push_back(BillboardEffectRenderElement{
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
    BoostTrailEffectSystem::BoostTrailEffectSystem() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void BoostTrailEffectSystem::emit(const BoostTrailEffectSpawnParams& params)
    {
        p_impl->Emit(params);
    }

    void BoostTrailEffectSystem::onRegistered()
    {
        p_impl->m_renderer.init(Asset_image::particle, boostTrailCapacity);
    }

    void BoostTrailEffectSystem::update(const RaceEffectFrameContext& context)
    {
        p_impl->Update(context);
    }

    void BoostTrailEffectSystem::drawTransparent() const
    {
        p_impl->m_renderer.draw();
    }

    void BoostTrailEffectSystem::onUnregistered()
    {
        p_impl->m_particles.clear();
        p_impl->m_renderer.finalize();
    }

    DriftSparkEffectSystem::DriftSparkEffectSystem() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void DriftSparkEffectSystem::emit(const DriftSparkEffectSpawnParams& params)
    {
        p_impl->Emit(params);
    }

    void DriftSparkEffectSystem::onRegistered()
    {
        p_impl->m_renderer.init(Asset_image::spark_01, driftSparkCapacity);
    }

    void DriftSparkEffectSystem::update(const RaceEffectFrameContext& context)
    {
        p_impl->Update(context);
    }

    void DriftSparkEffectSystem::drawTransparent() const
    {
        p_impl->m_renderer.draw();
    }

    void DriftSparkEffectSystem::onUnregistered()
    {
        p_impl->m_particles.clear();
        p_impl->m_renderer.finalize();
    }

    CollisionRingEffectSystem::CollisionRingEffectSystem() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void CollisionRingEffectSystem::emit(const CollisionRingEffectSpawnParams& params)
    {
        p_impl->Emit(params);
    }

    void CollisionRingEffectSystem::onRegistered()
    {
        p_impl->m_renderer.init(Asset_image::flame_01, collisionRingCapacity);
    }

    void CollisionRingEffectSystem::update(const RaceEffectFrameContext& context)
    {
        p_impl->Update(context);
    }

    void CollisionRingEffectSystem::drawTransparent() const
    {
        p_impl->m_renderer.draw();
    }

    void CollisionRingEffectSystem::onUnregistered()
    {
        p_impl->m_particles.clear();
        p_impl->m_renderer.finalize();
    }
}
