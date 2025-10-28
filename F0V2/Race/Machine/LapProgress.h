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

        LapProgress() = default;

        LapProgress(int lapIndex_, int segmentIndex_, int stripIndex_);

        [[nodiscard]]
        SegmentAndStrip segmentAndStrip() const;

        [[nodiscard]]
        bool isLessThan(LapProgress other) const;

        [[nodiscard]]
        bool operator==(const LapProgress&) const = default;
    };

    LapProgress EvaluateLapProgress(const LapProgress& previousLap, const SegmentAndStrip& currentIndex);
}
