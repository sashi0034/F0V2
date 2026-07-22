#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace Race
{
    enum class RaceCenterBanner : uint8_t
    {
        None,
        Go,
        YouGotBoostPower,
        TheFinalLap,
        Finish,
        YourMachineHasCrashed,
    };

    struct RaceControlState
    {
        int m_startCountdown{};
        RaceCenterBanner m_centerBanner{RaceCenterBanner::None};
        std::u32string m_centerBannerMessage{};
        int m_playerFinalRank{-1};
        bool m_raceFinished{};
        std::array<float, 3> m_measuredLapTimes{};
        bool m_isReversing{};
        bool m_boostTutorialEnabled{};
    };
}
