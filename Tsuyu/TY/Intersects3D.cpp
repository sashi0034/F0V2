#include "pch.h"
#include "Intersects3D.h"

using namespace TY;

namespace
{
    constexpr float EPS = 1e-6;
}

namespace
{
    bool intersectsInternal(const Line3D& line, const Triangle3D& tri, Float3* outIntersection)
    {
        const auto& [p0, p1] = line;
        const auto& [v0, v1, v2] = tri;

        Float3 dir = p1 - p0;
        Float3 edge1 = v1 - v0;
        Float3 edge2 = v2 - v0;

        Float3 h = dir.cross(edge2);
        float a = edge1.dot(h);
        if (Abs(a) < EPS) return false; // 平行

        float f = 1.0f / a;
        Float3 s = p0 - v0;
        float u = f * s.dot(h);
        if (u < 0.0f || u > 1.0f) return false;

        Float3 q = s.cross(edge1);
        float v = f * dir.dot(q);
        if (v < 0.0f || u + v > 1.0f) return false;

        float t = f * edge2.dot(q);
        if (t < 0.0f || t > 1.0f) return false; // 線分範囲外

        if (outIntersection)
        {
            *outIntersection = p0 + dir * t;
        }

        return true;
    }
}

bool TY::Intersects(const Line3D& line, const Triangle3D& tri)
{
    return intersectsInternal(line, tri, nullptr);
}

std::optional<Float3> TY::IntersectsAt(const Line3D& line, const Triangle3D& tri)
{
    Float3 intersection;
    if (intersectsInternal(line, tri, &intersection))
    {
        return intersection;
    }
    else
    {
        return std::nullopt;
    }
}

float TY::DistanceSq(const Line3D& lhs, const Line3D& rhs)
{
    const auto& [a, b] = lhs;
    const auto& [c, d] = rhs;

    const Float3 u = b - a;
    const Float3 v = d - c;
    const Float3 w = a - c;

    const float a_dot = u.dot(u);
    const float b_dot = u.dot(v);
    const float c_dot = v.dot(v);
    const float d_dot = u.dot(w);
    const float e_dot = v.dot(w);

    const float denom = a_dot * c_dot - b_dot * b_dot;
    float sc, sN, sD = denom;
    float tc, tN, tD = denom;

    if (denom < EPS)
    {
        sN = 0.0f;
        sD = 1.0f;
        tN = e_dot;
        tD = c_dot;
    }
    else
    {
        sN = (b_dot * e_dot - c_dot * d_dot);
        tN = (a_dot * e_dot - b_dot * d_dot);
        if (sN < 0.0f)
        {
            sN = 0.0f;
            tN = e_dot;
            tD = c_dot;
        }
        else if (sN > sD)
        {
            sN = sD;
            tN = e_dot + b_dot;
            tD = c_dot;
        }
    }

    sc = (std::abs(sN) < EPS ? 0.0f : sN / sD);
    tc = (std::abs(tN) < EPS ? 0.0f : tN / tD);

    Float3 dP = w + (u * sc) - (v * tc);
    return dP.lengthSq();
}

Float3 TY::ClosestPoint(const Float3& p, const Line3D& line)
{
    const auto& [a, b] = line;
    const Float3 ab = b - a;
    float t = (p - a).dot(ab) / ab.lengthSq();
    t = Math::Clamp(t, 0.0f, 1.0f);
    return a + ab * t;
}

bool TY::Intersects(const Triangle3D& tri, const Line3D& line)
{
    return Intersects(line, tri);
}

bool TY::Intersects(const Triangle3D& tri, const Capsule& capsule)
{
    const auto& [v0, v1, v2] = tri;
    const auto& [p0, p1, r] = capsule;

    // まず「カプセル中心線分 vs 三角形」の距離を計算
    // 三角形を辺ごとに線分とみなして最短距離を計算
    float r2 = r * r;
    float dist2 = std::numeric_limits<float>::max();

    dist2 = std::min(dist2, DistanceSq(Line3D{p0, p1}, Line3D{v0, v1}));
    dist2 = std::min(dist2, DistanceSq(Line3D{p0, p1}, Line3D{v1, v2}));
    dist2 = std::min(dist2, DistanceSq(Line3D{p0, p1}, Line3D{v2, v0}));

    // 三角形面に垂線を下ろす場合も考慮
    Float3 triNormal = (v1 - v0).cross(v2 - v0);
    float lenN = (triNormal).length();
    if (lenN > EPS)
    {
        triNormal = triNormal * (1.0f / lenN);
        // カプセル線分の任意点から三角形面への最近接点
        if (Intersects(Triangle3D{p0, p1, v0}, Line3D{v1, v2}))
        {
            // 線分と三角形平面が交差
            return true;
        }
    }

    return dist2 <= r2;
}

bool TY::Intersects(const Capsule& capsule, const Triangle3D& tri)
{
    return Intersects(tri, capsule);
}
