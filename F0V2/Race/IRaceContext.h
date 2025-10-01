#pragma once

namespace Race
{
    struct RaceContextPayload;

    class StageManager;

    class IRaceContext
    {
    public:
        virtual ~IRaceContext() = default;

        virtual RaceContextPayload& state() = 0;
        virtual const RaceContextPayload& state() const = 0;

        virtual StageManager& stageManager() = 0;
        virtual const StageManager& stageManager() const = 0;
    };

    IRaceContext& GetRaceContext();

    RaceContextPayload& GetRaceContextPayload();
}
