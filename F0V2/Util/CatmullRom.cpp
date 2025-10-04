#include "pch.h"
#include "CatmullRom.h"

namespace
{
    float UnwrapAngle(float angle, float reference)
    {
        float diff = std::fmod(angle - reference + Math::Pi_v<float>, Math::TwoPi_v<float>) - Math::Pi_v<float>;
        return reference + diff;
    }
}

Float3 Util::CatmullRom(const Float3& p0, const Float3& p1, const Float3& p2, const Float3& p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    return (p1 * 2.0f +
        (p2 - p0) * t +
        (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
        (-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3) * 0.5f;
}

Array<Float3> Util::GenerateCatmullRomPoints(const Float3& p0, const Float3& p1, const Float3& p2, const Float3& p3,
                                             int samplesPerSegment)
{
    Array<Float3> result;

    result.push_back(p1);

    for (int j = 1; j < samplesPerSegment; ++j)
    {
        float t = static_cast<float>(j) / samplesPerSegment;
        result.push_back(CatmullRom(p0, p1, p2, p3, t));
    }

    result.push_back(p2);

    return result;
}

float Util::CatmullRomAngle(float p0, float p1, float p2, float p3, float t)
{
    // p1 を基準に角度を連続化
    p0 = UnwrapAngle(p0, p1);
    p2 = UnwrapAngle(p2, p1);
    p3 = UnwrapAngle(p3, p1);

    // 通常の Catmull-Rom 式
    float t2 = t * t;
    float t3 = t2 * t;
    float result = (p1 * 2.0f +
        (p2 - p0) * t +
        (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
        (-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3) * 0.5f;

    // [-π, π) に正規化して返す
    result = std::fmod(result + Math::Pi_v<float>, Math::TwoPi_v<float>);
    if (result < 0.0f)
        result += Math::TwoPi_v<float>;
    result -= Math::Pi_v<float>;

    return result;
}

Array<float> Util::GenerateCatmullRomAngles(float p0, float p1, float p2, float p3, int samplesPerSegment)
{
    Array<float> result;

    result.push_back(p1);

    for (int j = 1; j < samplesPerSegment; ++j)
    {
        float t = static_cast<float>(j) / samplesPerSegment;
        result.push_back(CatmullRomAngle(p0, p1, p2, p3, t));
    }

    result.push_back(p2);

    return result;
}
