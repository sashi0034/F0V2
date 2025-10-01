#pragma once

namespace Race
{
    struct RaceContextState;

    class StageManager;

    class IRaceContext
    {
    public:
        virtual ~IRaceContext() = default;

        virtual RaceContextState& state() = 0;
        virtual const RaceContextState& state() const = 0;

        virtual StageManager& stageManager() = 0;
        virtual const StageManager& stageManager() const = 0;
    };

    IRaceContext& GetRaceContext();

    RaceContextState& GetRaceContextState();
}
