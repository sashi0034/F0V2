#include "pch.h"
#include "MachineEventHandler.h"

#include "MachinePhysics.h"
#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/EaseActor.h"
#include "TY/Easing.h"
#include "TY_Extension/AwaitContext.h"

using namespace Race;

namespace
{
}

struct MachineEventHandler::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"MachineEventHandler";
#endif
    ActorContainer m_children{};

    Array<ActorLifetimeScope> m_machineProcessList{};

    void Init()
    {
        m_machineProcessList.resize(MaxMachineCount);
    }

    void HandleIfNeeded(MachineId id)
    {
        auto& machine = GetRaceContext().machineManager().fetchMachine(id);
        if (machine.state.m_isRunningEventProcess)
        {
            return;
        }

        if (machine.state.m_isFallingOffCourse)
        {
            getMachineProcess(id).clear();
            StartCoroutine(m_children, [this, &machine, id](AwaitContext& await)
            {
                machine.state.m_isRunningEventProcess = true;

                handleFallingOffCourse(await, machine.state, machine.props);

                assert(machine.state.m_isFallingOffCourse);
                machine.state.m_isRunningEventProcess = false;
            }) >> getMachineProcess(id);
        }
    }

private:
    ActorLifetimeScope& getMachineProcess(MachineId id)
    {
        return m_machineProcessList[id];
    }

    void update() override
    {
        const auto& machineList = GetRaceContext().machineManager().machineList();
        for (MachineId id = 0; id < machineList.size(); ++id)
        {
            if (not machineList[id].state.m_isRunningEventProcess)
            {
                m_machineProcessList[id].clear();
            }
        }

        m_children.updateEach();
    }

    static SegmentAndStrip getFallRecoveryPosition(const MachinePhysicsState& state)
    {
        const auto targetSegmentAntStrip = state.m_lastGroundContactLocation;

        // TODO: 崖付近から離れるようにしたい

        return targetSegmentAntStrip;
    }

    void handleFallingOffCourse(AwaitContext& await, MachinePhysicsState& state, const MachinePhysicsProps& props)
    {
        float rate{};
        const auto fromPose = state.m_pose;

        const auto& segments = GetRaceContext().stageManager().courseSegments();

        const auto& targetSegmentAntStrip = getFallRecoveryPosition(state);

        const auto& targetStrip =
            segments[targetSegmentAntStrip.segmentIndex].midwayStrips[targetSegmentAntStrip.stripIndex];

        float targetElevation = 5.0f;
        if (targetStrip.style == CourseSegmentStyle::Cylinder)
        {
            targetElevation += CylinderBaseRadius;
        }

        const Float3 toPosition =
            (targetStrip.leftmost + targetStrip.rightmost) * 0.5f + targetStrip.normal * targetElevation;

        const Quaternion toRotation =
            Quaternion::FromUnitVectors(Float3{0, 0, 1}, targetStrip.toNext.normalized());

        state.m_forwardVector = targetStrip.toNext.normalized();
        state.m_upVector = targetStrip.normal;
        state.m_velocity = {};
        state.m_durability = PositiveF32{state.m_durability - 1000.0f};

        await.waitForExpired(
            StartEasing<EaseInOutBack>(m_children, rate, 1.0f, 3.0s)
            .onUpdate([&]
            {
                state.m_pose.position = Math::Lerp3D(fromPose.position, toPosition, rate);
                state.m_pose.rotation = fromPose.rotation.slerp(toRotation, rate);
            }) >> getMachineProcess(props.machineId)
        );
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Race
{
    MachineEventHandler::MachineEventHandler() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void MachineEventHandler::init()
    {
        p_impl->Init();
    }

    void MachineEventHandler::handleIfNeeded(MachineId id)
    {
        p_impl->HandleIfNeeded(id);
    }

    std::shared_ptr<ActorBase> MachineEventHandler::asActor() const
    {
        return p_impl;
    }
}
