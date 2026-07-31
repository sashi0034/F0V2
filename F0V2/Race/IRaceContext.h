#pragma once
#include "IRaceDrawer.h"
#include "Machine/MachineManager.h"
#include "Machine/MachinePhysicsUnit.h"

namespace Race
{
    struct RaceContextContent;

    struct CourseFileInfo;

    class StageManager;

    class RaceEffectDrawer;

    class SpatialAI;

    class CharacterAI;

    class MetaAI;

    class IRaceContext
    {
    public:
        virtual ~IRaceContext() = default;

        virtual RaceContextContent& state() = 0;
        virtual const RaceContextContent& state() const = 0;

        virtual const CourseFileInfo& courseFileInfo() const = 0;

        virtual void registerDrawer(const std::shared_ptr<IRaceDrawer>& drawer) = 0;
        virtual void unregisterDrawer(const IRaceDrawer* drawer) = 0;

        virtual RaceEffectDrawer& effectDrawer() = 0;
        virtual const RaceEffectDrawer& effectDrawer() const = 0;

        virtual StageManager& stageManager() = 0;
        virtual const StageManager& stageManager() const = 0;

        virtual MachineManager& machineManager() = 0;
        virtual const MachineManager& machineManager() const = 0;

        virtual SpatialAI& spatialAI() = 0;
        virtual const SpatialAI& spatialAI() const = 0;

        virtual Array<CharacterAI>& characterAIList() = 0;
        virtual const Array<CharacterAI>& characterAIList() const = 0;

        // virtual MetaAI& metaAI() = 0;
        // virtual const MetaAI& metaAI() const = 0;
    };

    IRaceContext& GetRaceContext();

    RaceContextContent& GetRaceContextContent();
}
