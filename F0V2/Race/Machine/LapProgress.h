#pragma once

namespace Race
{
    struct SegmentAndStrip
    {
        int segmentIndex{};
        int stripIndex{};
    };

    struct LapProgress
    {
        int lapIndex{};
        int segmentIndex{};
        int stripIndex{};

        SegmentAndStrip segmentAndStrip() const;
    };

    LapProgress EvaluateLapProgress(const LapProgress& previousLap, const SegmentAndStrip& currentIndex);
}
