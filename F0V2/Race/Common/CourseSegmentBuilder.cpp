#include "pch.h"
#include "CourseSegmentBuilder.h"

#include "CourseConstants.h"
#include "CourseData.h"
#include "TY/Array.h"
#include "TY/Math.h"
#include "TY/Quaternion.h"
#include "TY/Vector3D.h"
#include "Util/CatmullRom.h"

namespace Race
{
    Array<int> BuildCourseSegmentIfNeeded(
        Array<CourseSegment>& segments,
        const Array<CourseNode>& nodeList)
    {
        Array<int> rebuildIndexes{};
        for (int i = 0; i < nodeList.size(); ++i)
        {
            int i0 = Modulo<int>(i - 1, nodeList.size());
            int i1 = i;
            int i2 = Modulo<int>(i + 1, nodeList.size());
            int i3 = Modulo<int>(i + 2, nodeList.size());

            const Float3& p0 = nodeList[i0].pos;
            const Float3& p1 = nodeList[i1].pos;
            const Float3& p2 = nodeList[i2].pos;
            const Float3& p3 = nodeList[i3].pos;

            const float p0_roll = nodeList[i0].rollRadians();
            const float p1_roll = nodeList[i1].rollRadians();
            const float p2_roll = nodeList[i2].rollRadians();
            const float p3_roll = nodeList[i3].rollRadians();

            const auto style = nodeList[i1].style;

            if (i >= segments.size() ||
                segments[i].side_p0 != p0 ||
                segments[i].p1 != p1 ||
                segments[i].p2 != p2 ||
                segments[i].side_p3 != p3 ||
                segments[i].side_p0_roll != p0_roll ||
                segments[i].p1_roll != p1_roll ||
                segments[i].p2_roll != p2_roll ||
                segments[i].side_p3_roll != p3_roll ||
                segments[i].style != style ||
                segments[i].gimmicks != nodeList[i1].gimmicks)
            {
                if (i >= segments.size())
                {
                    segments.push_back({});
                }

                auto& segment = segments[i];
                segment.side_p0_roll = p0_roll;
                segment.p1 = p1;
                segment.p2 = p2;
                segment.side_p3_roll = p3_roll;

                segment.side_p0 = p0;
                segment.p1_roll = p1_roll;
                segment.p2_roll = p2_roll;
                segment.side_p3 = p3;

                segment.style = style;

                segment.gimmicks = nodeList[i1].gimmicks;

                rebuildIndexes.push_back(i0);
                rebuildIndexes.push_back(i1);
                rebuildIndexes.push_back(i2);
                rebuildIndexes.push_back(i3);
            }
        }

        std::ranges::sort(rebuildIndexes);

        // 重複を除去
        {
            auto last = std::ranges::unique(rebuildIndexes);
            rebuildIndexes.erase(last.begin(), last.end());
        }

        while (segments.size() > nodeList.size())
        {
            segments.pop_back();
        }

        // -----------------------------------------------

        // 変更があった CourseSegment の線分に対して面を構築する
        for (const auto i : rebuildIndexes)
        {
            const auto& priorSegment = segments[Modulo<int>(i - 1, segments.size())];
            const auto& nextSegment = segments[(i + 1) % segments.size()];

            auto& segment = segments[i];
            segment.midwayStrips.clear();

            const int samplesPerSegment = (segment.p2 - segment.p1).length() / 5.0f;
            const auto midwayPositions = Util::GenerateCatmullRomPoints(
                segment.side_p0, segment.p1, segment.p2, segment.side_p3, samplesPerSegment);
            const auto midwayRolls = Util::GenerateCatmullRomAngles(
                segment.side_p0_roll, segment.p1_roll, segment.p2_roll, segment.side_p3_roll, samplesPerSegment);

            // midwayPositions ごとに strip を構築
            for (int m = 0; m < midwayPositions.size() - 1 /* 終端は除外 */; ++m)
            {
                CourseStrip strip{};
                strip.center = midwayPositions[m];

                const float roll = midwayRolls[m];

                auto nextPosition = midwayPositions[m + 1];
                strip.toNext = nextPosition - strip.center;
                strip.lengthToNext = strip.toNext.length();

                {
                    const Float3 n = strip.toNext.cross(Float3(0, 1, 0));
                    strip.normal = n.cross(strip.toNext); // 鉛直上ベクトルと進行方向に垂直なベクトル

                    const auto q = Quaternion(strip.toNext.normalized(), roll);
                    strip.normal = q.rotate(strip.normal).normalized();
                }

                auto right = strip.toNext.cross(strip.normal).normalized();
                float width = 12.5f; // TODO
                strip.leftmost = strip.center - right * width;
                strip.rightmost = strip.center + right * width;

                if (segment.style == CourseSegmentStyle::Pipe)
                {
                    for (int t = 0; t < PipeSubdivision; ++t)
                    {
                        // 円周上の方向ベクトルを計算
                        const float angle =
                            Math::HalfPiF - (0.5 + static_cast<float>(t) / PipeSubdivision) * Math::TwoPi_v<float>;
                        const Float3 dir = Quaternion(strip.toNext.normalized(), angle).rotate(strip.normal);
                        strip.pipe.ringVectors[t] = dir;
                    }
                }
                else if (segment.style == CourseSegmentStyle::Cylinder)
                {
                    for (int t = 0; t < CylinderSubdivision; ++t)
                    {
                        // 円周上の方向ベクトルを計算
                        const float angle =
                            Math::HalfPiF - (0.5 + static_cast<float>(t) / CylinderSubdivision) * Math::TwoPi_v<float>;
                        const Float3 dir = Quaternion(strip.toNext.normalized(), angle).rotate(strip.normal);
                        strip.pipe.ringVectors[t] = dir;
                    }
                }

                // -----------------------------------------------
                // <-- strip.style

                strip.style = segment.style;

                if (strip.style == CourseSegmentStyle::Pipe)
                {
                    if (priorSegment.style != CourseSegmentStyle::Pipe &&
                        m < PipeEntryExitStrips)
                    {
                        // 入口
                        strip.style = CourseSegmentStyle::Road;
                    }
                    else if (nextSegment.style != CourseSegmentStyle::Pipe &&
                        m >= midwayPositions.size() - PipeEntryExitStrips)
                    {
                        // 出口
                        strip.style = CourseSegmentStyle::Road;
                    }
                }
                else if (strip.style == CourseSegmentStyle::Cylinder)
                {
                    if (priorSegment.style != CourseSegmentStyle::Cylinder &&
                        m < CylinderEntryExitStrips / 2)
                    {
                        // 入口
                        strip.style = CourseSegmentStyle::Road;
                    }
                    else if (nextSegment.style != CourseSegmentStyle::Cylinder &&
                        m >= midwayPositions.size() - CylinderEntryExitStrips / 2)
                    {
                        // 出口
                        strip.style = CourseSegmentStyle::Road;
                    }
                }

                // --> strip.style
                // -----------------------------------------------

                segment.midwayStrips.push_back(strip);
            }

            // 終端部分は次のセクションで行う
        }

        for (const auto i : rebuildIndexes)
        {
            auto& segment = segments[i];

            // 終端部分の追加
            {
                // FIXME?
                auto& segment1 = segments[(i + 1) % segments.size()];
                segment.midwayStrips.push_back(segment1.midwayStrips[0]);
            }

            segment.totalLength = 0.0f;
            for (int m = 0; m < segment.midwayStrips.size(); ++m)
            {
                segment.totalLength += segment.midwayStrips[m].lengthToNext;
            }
        }

        return rebuildIndexes;
    }
}
