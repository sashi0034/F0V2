#pragma once

namespace Race
{
    struct RaceContextState;

    class IRaceContext
    {
    public:
        virtual ~IRaceContext() = default;

        virtual RaceContextState& state() = 0;
        virtual const RaceContextState& state() const = 0;
    };

    IRaceContext& GetRaceContext();

    // TODO: g_contextState にするかも
    RaceContextState& GetRaceContextState();
}
