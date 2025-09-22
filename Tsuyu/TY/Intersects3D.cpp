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

Float3 TY::ClosestPoint(const Float3& p, const Line3D& line)
{
    const auto& [a, b] = line;
    const Float3 ab = b - a;
    float t = (p - a).dot(ab) / ab.lengthSq();
    t = Math::Clamp(t, 0.0f, 1.0f);
    return a + ab * t;
}

float TY::DistanceSq(const Float3& p, const Line3D& line)
{
    Float3 closest = ClosestPoint(p, line);
    return (p - closest).lengthSq();
}

float TY::DistanceSq(const Float3& p, const Triangle3D& tri)
{
    const Float3 a = tri.p0, b = tri.p1, c = tri.p2;
    const Float3 ab = b - a;
    const Float3 ac = c - a;
    const Float3 ap = p - a;

    float d1 = ab.dot(ap);
    float d2 = ac.dot(ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return (p - a).lengthSq(); // A 頂点

    const Float3 bp = p - b;
    float d3 = ab.dot(bp);
    float d4 = ac.dot(bp);
    if (d3 >= 0.0f && d4 <= d3) return (p - b).lengthSq(); // B 頂点

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float v = d1 / (d1 - d3);
        Float3 proj = a + ab * v;
        return (p - proj).lengthSq(); // AB 辺上
    }

    const Float3 cp = p - c;
    float d5 = ab.dot(cp);
    float d6 = ac.dot(cp);
    if (d6 >= 0.0f && d5 <= d6) return (p - c).lengthSq(); // C 頂点

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        float w = d2 / (d2 - d6);
        Float3 proj = a + ac * w;
        return (p - proj).lengthSq(); // AC 辺上
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        Float3 proj = b + (c - b) * w;
        return (p - proj).lengthSq(); // BC 辺上
    }

    // 面内部
    Float3 n = ab.cross(ac);
    float dist = (p - a).dot(n) / std::sqrt(std::max(1e-30f, n.lengthSq()));
    return dist * dist;
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

    if (std::abs(denom) < EPS)
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

    if (tN < 0.0f)
    {
        tN = 0.0f;
        if (-d_dot < 0.0f)
        {
            sN = 0.0f;
        }
        else if (-d_dot > a_dot)
        {
            sN = sD;
        }
        else
        {
            sN = -d_dot;
            sD = a_dot;
        }
    }
    else if (tN > tD)
    {
        tN = tD;
        if ((-d_dot + b_dot) < 0.0f)
        {
            sN = 0.0f;
        }
        else if ((-d_dot + b_dot) > a_dot)
        {
            sN = sD;
        }
        else
        {
            sN = (-d_dot + b_dot);
            sD = a_dot;
        }
    }

    sc = (std::abs(sD) < EPS ? 0.0f : sN / sD);
    tc = (std::abs(tD) < EPS ? 0.0f : tN / tD);

    Float3 dP = w + (u * sc) - (v * tc);
    return dP.lengthSq();
}

float TY::DistanceSq(const Line3D& line, const Triangle3D& tri)
{
    if (Intersects(line, tri))
    {
        return 0.0f;
    }

    float best = FLT_MAX;
    best = Min(best, DistanceSq(line.p0, tri));
    best = Min(best, DistanceSq(line.p1, tri));

    Line3D e0{tri.p0, tri.p1};
    Line3D e1{tri.p1, tri.p2};
    Line3D e2{tri.p2, tri.p0};

    best = Min(best, DistanceSq(line, e0));
    best = Min(best, DistanceSq(line, e1));
    best = Min(best, DistanceSq(line, e2));

    return best;
}

bool TY::Intersects(const Triangle3D& tri, const Line3D& line)
{
    return Intersects(line, tri);
}

bool TY::Intersects(const Triangle3D& tri, const Capsule& capsule)
{
    const float r2 = capsule.radius * capsule.radius;
    const float dist2 = DistanceSq(Line3D{capsule.p0, capsule.p1}, tri);
    return dist2 <= r2;
}

bool TY::Intersects(const Capsule& capsule, const Triangle3D& tri)
{
    return Intersects(tri, capsule);
}
