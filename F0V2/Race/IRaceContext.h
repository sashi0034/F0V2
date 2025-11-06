#pragma once
#include "IRaceDrawer.h"
#include "Machine/MachineManager.h"
#include "Machine/MachinePhysicsUnit.h"

namespace Race
{
    struct RaceContextContent;

    class StageManager;

    struct SpatialAi;

    class IRaceContext
    {
    public:
        virtual ~IRaceContext() = default;

        virtual RaceContextContent& state() = 0;
        virtual const RaceContextContent& state() const = 0;

        virtual void registerDrawer(const std::shared_ptr<IRaceDrawer>& drawer) = 0;
        virtual void unregisterDrawer(const IRaceDrawer* drawer) = 0;

        virtual StageManager& stageManager() = 0;
        virtual const StageManager& stageManager() const = 0;

        virtual MachineManager& machineManager() = 0;
        virtual const MachineManager& machineManager() const = 0;

        virtual SpatialAi& spatialAi() = 0;
        virtual const SpatialAi& spatialAi() const = 0;
    };

    IRaceContext& GetRaceContext();

    RaceContextContent& GetRaceContextContent();
}
