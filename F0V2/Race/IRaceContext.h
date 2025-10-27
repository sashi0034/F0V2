#pragma once
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

        virtual StageManager& stageManager() = 0;
        virtual const StageManager& stageManager() const = 0;

        virtual SpatialAi& spatialAi() = 0;
        virtual const SpatialAi& spatialAi() const = 0;

        // virtual MachineUnit& getMachine(int id) = 0;
        virtual const MachinePhysicsUnit& getMachine(int id) const = 0;
    };

    IRaceContext& GetRaceContext();

    RaceContextContent& GetRaceContextContent();
}
