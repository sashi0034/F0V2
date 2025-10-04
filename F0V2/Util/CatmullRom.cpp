#include "pch.h"
#include "CatmullRom.h"

Float3 Util::CatmullRomPoint(const Float3& p0, const Float3& p1, const Float3& p2, const Float3& p3, float t)
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
        result.push_back(CatmullRomPoint(p0, p1, p2, p3, t));
    }

    result.push_back(p2);

    return result;
}

namespace
{
    float UnwrapNear(float a, float ref)
    {
        // 対称ラップ（[-π, π] に収める差分）std::remainder は負にも自然
        return ref + std::remainder(a - ref, Math::TwoPi_v<float>);
    }
}

float Util::CatmullRomAngle(float p0, float p1, float p2, float p3, float t)
{
    // 区間は p1 --> p2
    // 始点側は p1 基準、終点側の p3 は p2 基準で連続化する
    const float a1 = p1;
    const float a0 = UnwrapNear(p0, a1);
    const float a2 = UnwrapNear(p2, a1);
    const float a3 = UnwrapNear(p3, a2); // <-- p2 基準

    const float t2 = t * t;
    const float t3 = t2 * t;

    float result = (a1 * 2.0f +
        (a2 - a0) * t +
        (a0 * 2.0f - a1 * 5.0f + a2 * 4.0f - a3) * t2 +
        (-a0 + a1 * 3.0f - a2 * 3.0f + a3) * t3) * 0.5f;

    // 出力は好きなレンジへ正規化（[-π, π) なら以下）
    result = std::remainder(result, Math::TwoPi_v<float>);

    // remainder は [-π, π]なので [-π, π) にしたい場合だけ調整
    if (result == Math::Pi_v<float>)
    {
        result = -Math::Pi_v<float>;
    }

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
