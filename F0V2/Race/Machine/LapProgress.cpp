#include "pch.h"
#include "LapProgress.h"

#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"

using namespace Race;

namespace
{
    float evaluateTraveledDistance(const SegmentAndStrip& from, const SegmentAndStrip& to)
    {
        float length{};

        const auto& segments = GetRaceContext().stageManager().courseSegments();

        SegmentAndStrip cursor = from;
        if (cursor.segmentIndex == to.segmentIndex)
        {
            for (int i = cursor.stripIndex; i < to.stripIndex; ++i)
            {
                length += segments[cursor.segmentIndex].midwayStrips[cursor.stripIndex].lengthToNext;
            }

            for (int i = cursor.stripIndex; i > to.stripIndex; --i)
            {
                length -= segments[cursor.segmentIndex].midwayStrips[cursor.stripIndex].lengthToNext;
            }

            return length;
        }

        for (; cursor.stripIndex < segments[cursor.segmentIndex].midwayStrips.size(); ++cursor.stripIndex)
        {
            length += segments[cursor.segmentIndex].midwayStrips[cursor.stripIndex].lengthToNext;
        }

        cursor.segmentIndex = (cursor.segmentIndex + 1) % segments.size();
        cursor.stripIndex = 0;

        while (cursor.segmentIndex != to.segmentIndex)
        {
            length += segments[cursor.segmentIndex].totalLength;
            cursor.segmentIndex = (cursor.segmentIndex + 1) % segments.size();
        }

        for (; cursor.stripIndex < to.stripIndex; ++cursor.stripIndex)
        {
            length += segments[cursor.segmentIndex].midwayStrips[cursor.stripIndex].lengthToNext;
        }

        const float courseLength = GetRaceContext().stageManager().courseLength();

        // コース半分の距離分は一度に進めないと仮定し、その場合は逆走として扱う
        if (length > courseLength * 0.5f)
        {
            assert(
                length - courseLength <= 0.0f &&
                "evaluateTraveledDistance(): Invalid distance calculation exceeds course length");
            return length - courseLength;
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
