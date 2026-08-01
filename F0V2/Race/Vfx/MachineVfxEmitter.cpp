#include "pch.h"
#include "MachineVfxEmitter.h"

#include "MachineVfxSystems.h"
#include "RaceVfxDrawer.h"
#include "Race/IRaceContext.h"
#include "Race/Machine/MachineConstants.h"
#include "Race/Machine/MachinePhysicsUnit.h"
#include "TY/Array.h"
#include "TY/GameTime.h"
#include "TY/Periodic.h"

using namespace Race;

namespace
{
    constexpr float DriftEmitInterval = 0.05f;
    constexpr float DriftThreshold = 0.05f;

    struct StatePerMachine
    {
        float boostIntensity{};
        Float3 lastBoostEmitPosition{};
        float driftEmitCountdown{};
        float previousAttackedTime{};
        bool initialized{};
    };
}

struct MachineVfxEmitter::Impl : ActorBase
{
    Array<StatePerMachine> m_machineStates{MaxMachineCount};

    std::shared_ptr<BoostTrailVfxSystem> m_boostTrailSystem{};
    std::shared_ptr<DriftSparkVfxSystem> m_driftSparkSystem{};
    std::shared_ptr<CollisionRingVfxSystem> m_collisionRingSystem{};

    void Init()
    {
        m_boostTrailSystem = std::make_shared<BoostTrailVfxSystem>();
        m_driftSparkSystem = std::make_shared<DriftSparkVfxSystem>();
        m_collisionRingSystem = std::make_shared<CollisionRingVfxSystem>();

        auto& vfxDrawer = GetRaceContext().vfxDrawer();
        vfxDrawer.registerVfxSystem(m_boostTrailSystem);
        vfxDrawer.registerVfxSystem(m_driftSparkSystem);
        vfxDrawer.registerVfxSystem(m_collisionRingSystem);
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

            emitBoostParticles(machine, state);
            emitDriftParticles(machine, state);
            emitCollisionParticle(machine, state);
        }
    }

    void emitBoostParticles(const MachinePhysicsUnit& machine, StatePerMachine& state) const
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
            const Float3 particlePosition =
                state.lastBoostEmitPosition + emitDirection * emitThreshold * (j + 1) +
                emitRight * (0.5f * Periodic::Sine1_1(0.15s, InGameElapsedTime()));

            m_boostTrailSystem->emit(BoostTrailVfxSpawnParams{
                .worldPosition = particlePosition,
                .color = machine.props.themeColor,
                .intensity = state.boostIntensity,
            });
        }

        state.lastBoostEmitPosition = emitPosition;
    }

    void emitDriftParticles(const MachinePhysicsUnit& machine, StatePerMachine& state) const
    {
        state.driftEmitCountdown = Max(0.0f, state.driftEmitCountdown - InGameDeltaTime());

        const bool shouldEmit =
            not machine.state.isHovering() &&
            Abs(machine.state.m_driftOffset) >= DriftThreshold;

        if (not shouldEmit)
        {
            state.driftEmitCountdown = 0.0f;
            return;
        }

        if (state.driftEmitCountdown > 0.0f)
        {
            return;
        }

        state.driftEmitCountdown = DriftEmitInterval;

        const Float3 rearCenter =
            machine.state.m_pose.position - machine.state.m_visualForwardVector * (MachineHeight * 0.4f);
        const Float3 velocity =
            -machine.state.m_visualForwardVector * 5.0f + machine.state.m_upVector * 2.0f;

        for (const float side : {-1.0f, 1.0f})
        {
            m_driftSparkSystem->emit(DriftSparkVfxSpawnParams{
                .worldPosition = rearCenter + machine.state.rightVector() * (MachineRadius * side),
                .velocity = velocity,
            });
        }
    }

    void emitCollisionParticle(const MachinePhysicsUnit& machine, StatePerMachine& state) const
    {
        const GimmickFlagBits newTouchingGimmicks =
            machine.state.m_touchingGimmicks & (~machine.state.m_previousTouchingGimmicks);
        const bool hitBarrier = newTouchingGimmicks & GimmickFlag::Barrier;
        const bool attackedByMachine =
            state.previousAttackedTime != machine.state.m_lastAttackedByOtherMachineTime;

        state.previousAttackedTime = machine.state.m_lastAttackedByOtherMachineTime;

        if (not hitBarrier && not attackedByMachine)
        {
            return;
        }

        m_collisionRingSystem->emit(CollisionRingVfxSpawnParams{
            .worldPosition = machine.state.m_pose.position,
            .color = machine.props.themeColor,
        });
    }

    float orderPriority() const override
    {
        return -100.0f;
    }

    void killed() override
    {
        auto& vfxDrawer = GetRaceContext().vfxDrawer();
        if (vfxDrawer.isAlive())
        {
            vfxDrawer.unregisterVfxSystem(m_collisionRingSystem.get());
            vfxDrawer.unregisterVfxSystem(m_driftSparkSystem.get());
            vfxDrawer.unregisterVfxSystem(m_boostTrailSystem.get());
        }

        m_collisionRingSystem.reset();
        m_driftSparkSystem.reset();
        m_boostTrailSystem.reset();
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
