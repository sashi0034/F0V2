#pragma once
#include "TY/Array.h"
#include "TY/Vector3D.h"

namespace Util
{
    float CatmullRomValue(float p0, float p1, float p2, float p3, float t);

    Array<float> GenerateCatmullRomValues(
        float p0, float p1, float p2, float p3, int samplesPerSegment);

    // Catmull-Rom 補間 (区間 p1 --> p2)
    Float3 CatmullRomPoint(const Float3& p0, const Float3& p1, const Float3& p2, const Float3& p3, float t);

    Array<Float3> GenerateCatmullRomPoints(
        const Float3& p0, const Float3& p1, const Float3& p2, const Float3& p3, int samplesPerSegment);

    float CatmullRomAngle(float p0, float p1, float p2, float p3, float t);

    Array<float> GenerateCatmullRomAngles(
        float p0, float p1, float p2, float p3, int samplesPerSegment);
}
