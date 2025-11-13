#include "pch.h"
#include "LapProgress.h"

#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"

using namespace Race;

namespace
{
    float evaluateTraveledDistance(const SegmentAndStrip& from, const SegmentAndStrip& to)
    {
        const auto& stageManager = GetRaceContext().stageManager();

        const float length = stageManager.getDistanceFromStart(to) - stageManager.getDistanceFromStart(from);

        const float courseLength = GetRaceContext().stageManager().courseLength();

        // コース半分の距離分は一度に進めないと仮定し、その場合は逆走として扱う
        if (length > courseLength * 0.5f)
        {
            return length - courseLength;
        }
        else if (length < -courseLength * 0.5f)
        {
            return length + courseLength;
        }

        return length;
    }
}

namespace Race
{
    LapProgress::LapProgress(int lapIndex_, int segmentIndex_, int stripIndex_)
        : lapIndex(lapIndex_), segmentIndex(segmentIndex_), stripIndex(stripIndex_)
    {
    }

    SegmentAndStrip LapProgress::segmentAndStrip() const
    {
        return {segmentIndex, stripIndex};
    }

    bool LapProgress::isLessThan(LapProgress other) const
    {
        return lapIndex < other.lapIndex
            || (lapIndex == other.lapIndex && segmentIndex < other.segmentIndex)
            || (lapIndex == other.lapIndex && segmentIndex == other.segmentIndex && stripIndex < other.stripIndex);
    }

    LapProgress EvaluateLapProgress(const LapProgress& previousLap, const SegmentAndStrip& currentIndex)
    {
        LapProgress result{};
        result.lapIndex = previousLap.lapIndex;
        result.segmentIndex = currentIndex.segmentIndex;
        result.stripIndex = currentIndex.stripIndex;

        if (previousLap.segmentIndex != currentIndex.segmentIndex)
        {
            const float traveled = evaluateTraveledDistance(previousLap.segmentAndStrip(), currentIndex);
            if (traveled < 0.0f)
            {
                if (previousLap.segmentIndex < currentIndex.segmentIndex)
                {
                    // 逆走
                    result.lapIndex = previousLap.lapIndex - 1;
                }
            }
            else
            {
                if (previousLap.segmentIndex > currentIndex.segmentIndex)
                {
                    // 周回完了
                    result.lapIndex = previousLap.lapIndex + 1;
                }
            }
        }
        else
        {
            result.lapIndex = previousLap.lapIndex;
        }

        return result;
    }
}
