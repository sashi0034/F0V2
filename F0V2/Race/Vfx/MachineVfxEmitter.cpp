#include "pch.h"
#include "MachineVfxEmitter.h"

#include "Asset.generated.h"
#include "BillboardVfxRenderer.h"
#include "IRaceVfxSystem.h"
#include "RaceVfxDrawer.h"
#include "SimpleParticleVfxRenderer.h"
#include "Race/IRaceContext.h"
#include "Race/Machine/MachineConstants.h"
#include "Race/Machine/MachinePhysicsUnit.h"
#include "TY/Array.h"
#include "TY/GameTime.h"
#include "TY/Periodic.h"

using namespace Race;

namespace
{
    struct StatePerMachine
    {
        float boostIntensity{};
        Float3 lastBoostEmitPosition{};
        float driftEmitCountdown{};
        float previousAttackedTime{};
        bool initialized{};
    };

    // -----------------------------------------------

    struct BoostVfx : IRaceVfxSystem
    {
        struct Particle
        {
            Float3 worldPosition{};
            ColorF32 color{};
            float scale{};
        };

        SimpleParticleVfxRenderer m_renderer{};
        Array<Particle> m_particles{};

        static constexpr int ParticleCapacity = 4096;

        void onRegistered() override
        {
            m_renderer.init(Asset_image::particle, ParticleCapacity);
        }

        void emitIfNeeded(const MachinePhysicsUnit& machine, StatePerMachine& state)
        {
            const bool isBoosting =
                machine.state.m_manualBoost > 0.0f || machine.state.m_passiveBoost > 0.0f;

            if (not isBoosting && state.boostIntensity <= 0.0f)
            {
                return;
            }

            if (isBoosting)
            {
                if (state.boostIntensity == 0.0f)
                {
                    state.lastBoostEmitPosition = machine.state.m_pose.position;
                }

                state.boostIntensity = 1.0f;
            }
            else
            {
                state.boostIntensity = Max(0.0f, state.boostIntensity - InGameDeltaTime());
                if (state.boostIntensity <= 0.0f)
                {
                    return;
                }
            }

            const Float3 emitPosition = machine.state.m_pose.position;
            const float emitThreshold = 1.0f - 0.1f * Periodic::Sine0_1(0.1s, InGameElapsedTime());
            const float distanceSinceLastEmit = (emitPosition - state.lastBoostEmitPosition).length();

            if (distanceSinceLastEmit < emitThreshold)
            {
                return;
            }

            const int emitCount = static_cast<int>(distanceSinceLastEmit / emitThreshold);
            const Float3 emitDirection = (emitPosition - state.lastBoostEmitPosition).normalized();
            const Float3 emitRight = machine.state.rightVector();

            for (int j = 0; j < emitCount; ++j)
            {
                if (m_particles.size() >= ParticleCapacity)
                {
                    break;
                }

                const Float3 particlePosition =
                    state.lastBoostEmitPosition + emitDirection * emitThreshold * (j + 1) +
                    emitRight * (0.5f * Periodic::Sine1_1(0.15s, InGameElapsedTime()));

                m_particles.push_back(Particle{
                    .worldPosition = particlePosition,
                    .color = machine.props.themeColor.withAlpha(1.0f),
                    .scale = 1.0f + state.boostIntensity,
                });
            }

            state.lastBoostEmitPosition = emitPosition;
        }

        void update(const RaceVfxFrameContext& context) override
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

        void drawTransparent() const override
        {
            m_renderer.draw();
        }

        void onUnregistered() override
        {
            m_particles.clear();
            m_renderer.finalize();
        }
    };

    // -----------------------------------------------

    struct DriftVfx : IRaceVfxSystem
    {
        struct Particle
        {
            MachineId targetMachineId{};
            Float3 relativePosition{};
            Float3 velocity{};
            float rotation{};
            float angularVelocity{8.0f};
            float age{};
            float lifetime{1.0f};
        };

        BillboardVfxRenderer m_renderer{};
        Array<Particle> m_particles{};

        static constexpr int ParticleCapacity = 2048;

        void onRegistered() override
        {
            m_renderer.init(Asset_image::spark_01, ParticleCapacity, GraphicsBlendOptions::Additive());
        }

        void emitIfNeeded(const MachinePhysicsUnit& machine, StatePerMachine& state)
        {
            state.driftEmitCountdown = Max(0.0f, state.driftEmitCountdown - InGameDeltaTime());

            constexpr float driftThreshold = 0.05f;
            const bool shouldEmit =
                not machine.state.isHovering() &&
                Abs(machine.state.m_driftOffset) >= driftThreshold;

            if (not shouldEmit)
            {
                state.driftEmitCountdown = 0.0f;
                return;
            }

            if (state.driftEmitCountdown > 0.0f)
            {
                return;
            }

            constexpr float driftEmitInterval = 0.1f;
            state.driftEmitCountdown = driftEmitInterval;

            for (const float side : {-1.0f, 1.0f})
            {
                if (m_particles.size() >= ParticleCapacity)
                {
                    break;
                }

                m_particles.push_back(Particle{
                    .targetMachineId = machine.id(),
                    .relativePosition = Float3{MachineRadius * side, 0.0f, -MachineHeight * 0.75f},
                    .velocity = -machine.state.m_gravity - machine.state.m_velocity.normalized(),
                });
            }
        }

        void update(const RaceVfxFrameContext& context) override
        {
            const ColorF32 startColor{1.0f, 0.75f, 0.2f, 1.0f};
            const ColorF32 endColor{1.0f, 0.3f, 0.05f, 0.0f};

            Array<BillboardVfxElement> renderElements{};
            renderElements.reserve(m_particles.size());
            const auto& machines = GetRaceContext().machineManager().machineList();

            for (int i = static_cast<int>(m_particles.size()) - 1; i >= 0; --i)
            {
                auto& particle = m_particles[i];
                particle.age += context.deltaTime;
                particle.relativePosition += particle.velocity * context.deltaTime;
                particle.rotation += particle.angularVelocity * context.deltaTime;
                if (
                    particle.age >= particle.lifetime ||
                    particle.targetMachineId < 0 ||
                    particle.targetMachineId >= machines.size())
                {
                    m_particles.remove_at(i);
                    continue;
                }

                const auto& machine = machines[particle.targetMachineId];
                const Float3 worldPosition =
                    machine.state.m_pose.position +
                    machine.state.rightVector() * particle.relativePosition.x +
                    machine.state.m_upVector * particle.relativePosition.y +
                    machine.state.m_visualForwardVector * particle.relativePosition.z;

                const float rate = Math::Clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
                const float scale = std::lerp(0.95f, 0.55f, rate);
                renderElements.push_back(BillboardVfxElement{
                    .worldPosition = worldPosition,
                    .rotation = particle.rotation,
                    .size = Float2{scale, scale},
                    .color = startColor.lerp(endColor, rate),
                });
            }

            m_renderer.upload(renderElements, context.cameraUp, context.cameraRight);
        }

        void drawTransparent() const override
        {
            m_renderer.draw();
        }

        void onUnregistered() override
        {
            m_particles.clear();
            m_renderer.finalize();
        }
    };

    // -----------------------------------------------

    struct CollisionVfx : IRaceVfxSystem
    {
        struct Particle
        {
            Float3 worldPosition{};
            float age{};
            float lifetime{0.3f};
            ColorF32 startColor{};
            ColorF32 endColor{};
        };

        BillboardVfxRenderer m_renderer{};
        Array<Particle> m_particles{};

        static constexpr int ParticleCapacity = 512;

        void onRegistered() override
        {
            m_renderer.init(Asset_image::flame_01, ParticleCapacity, GraphicsBlendOptions::Additive());
        }

        void emitIfNeeded(const MachinePhysicsUnit& machine, StatePerMachine& state)
        {
            const GimmickFlagBits newTouchingGimmicks =
                machine.state.m_touchingGimmicks & (~machine.state.m_previousTouchingGimmicks);
            const bool hitBarrier = newTouchingGimmicks & GimmickFlag::Barrier;
            const bool attackedByMachine =
                state.previousAttackedTime != machine.state.m_lastAttackedByOtherMachineTime;

            state.previousAttackedTime = machine.state.m_lastAttackedByOtherMachineTime;

            if ((not hitBarrier && not attackedByMachine) || m_particles.size() >= ParticleCapacity)
            {
                return;
            }

            m_particles.push_back(Particle{
                .worldPosition = machine.state.m_pose.position,
                .startColor = machine.props.themeColor,
                .endColor = machine.props.themeColor.withAlpha(0.0f),
            });
        }

        void update(const RaceVfxFrameContext& context) override
        {
            Array<BillboardVfxElement> renderElements{};
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
                renderElements.push_back(BillboardVfxElement{
                    .worldPosition = particle.worldPosition,
                    .size = Float2{scale, scale},
                    .color = particle.startColor.lerp(particle.endColor, rate),
                });
            }

            m_renderer.upload(renderElements, context.cameraUp, context.cameraRight);
        }

        void drawTransparent() const override
        {
            m_renderer.draw();
        }

        void onUnregistered() override
        {
            m_particles.clear();
            m_renderer.finalize();
        }
    };
}

struct MachineVfxEmitter::Impl : ActorBase
{
    Array<StatePerMachine> m_machineStates{MaxMachineCount};

    std::shared_ptr<BoostVfx> m_boostVfx{};
    std::shared_ptr<DriftVfx> m_driftVfx{};
    std::shared_ptr<CollisionVfx> m_collisionVfx{};

    void Init()
    {
        m_boostVfx = std::make_shared<BoostVfx>();
        m_driftVfx = std::make_shared<DriftVfx>();
        m_collisionVfx = std::make_shared<CollisionVfx>();

        auto& vfxDrawer = GetRaceContext().vfxDrawer();
        vfxDrawer.registerVfxSystem(m_boostVfx);
        vfxDrawer.registerVfxSystem(m_driftVfx);
        vfxDrawer.registerVfxSystem(m_collisionVfx);
    }

private:
    void update() override
    {
        const auto& machines = GetRaceContext().machineManager().machineList();
        for (int i = 0; i < machines.size(); ++i)
        {
            const auto& machine = machines[i];
            auto& state = m_machineStates[i];

            if (not state.initialized)
            {
                state.initialized = true;

                state.lastBoostEmitPosition = machine.state.m_pose.position;
                state.previousAttackedTime = machine.state.m_lastAttackedByOtherMachineTime;
            }

            m_boostVfx->emitIfNeeded(machine, state);
            m_driftVfx->emitIfNeeded(machine, state);
            m_collisionVfx->emitIfNeeded(machine, state);
        }
    }

    float orderPriority() const override
    {
        return -100.0f;
    }

    void killed() override
    {
        auto& vfxDrawer = GetRaceContext().vfxDrawer();
        vfxDrawer.unregisterVfxSystem(m_collisionVfx.get());
        vfxDrawer.unregisterVfxSystem(m_driftVfx.get());
        vfxDrawer.unregisterVfxSystem(m_boostVfx.get());
    }
};

namespace Race
{
    MachineVfxEmitter::MachineVfxEmitter() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void MachineVfxEmitter::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> MachineVfxEmitter::asActor() const
    {
        return p_impl;
    }
}
