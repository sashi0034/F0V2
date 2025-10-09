#include "pch.h"
#include "Intersects3D.h"

using namespace TY;

namespace
{
    constexpr float EPS_PARALLEL = 1e-8f;

    constexpr float EPS_ZERO = 1e-30f;
}

namespace
{
    bool intersectsInternal(const LineSegment3D& segment, const Triangle3D& tri, Float3* outIntersection)
    {
        const auto& [p0, p1] = segment;
        const auto& [v0, v1, v2] = tri;

        Float3 dir = p1 - p0;
        Float3 edge1 = v1 - v0;
        Float3 edge2 = v2 - v0;

        Float3 h = dir.cross(edge2);
        float a = edge1.dot(h);
        if (Abs(a) < EPS_PARALLEL) return false; // 平行

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
    const auto& [a, dir] = line;
    float t = (p - a).dot(dir) / dir.lengthSq();
    return a + dir * t;
}

Float3 TY::ClosestPoint(const Float3& p, const LineSegment3D& segment)
{
    const auto& [a, b] = segment;
    const Float3 ab = b - a;
    float t = (p - a).dot(ab) / ab.lengthSq();
    t = Math::Clamp(t, 0.0f, 1.0f);
    return a + ab * t;
}

float TY::DistanceSq(const Float3& lhs, const Float3& rhs)
{
    return (lhs - rhs).lengthSq();
}

float TY::DistanceSq(const Float3& p, const LineSegment3D& segment)
{
    Float3 closest = ClosestPoint(p, segment);
    return (p - closest).lengthSq();
}

float TY::DistanceSq(const Float3& p, const Plane3Points& plane)
{
    const Float3 A = plane.p0;
    const Float3 B = plane.p1;
    const Float3 C = plane.p2;
    const Float3 AB = B - A;
    const Float3 AC = C - A;
    const Float3 AP = p - A;

    const Float3 N = AB.cross(AC);
    const float APoN = AP.dot(N);
    return APoN * APoN / std::max(EPS_ZERO, N.lengthSq());
}

float TY::DistanceSq(const Float3& p, const Triangle3D& tri)
{
    const Float3 A = tri.p0, B = tri.p1, C = tri.p2;
    const Float3 AB = B - A;
    const Float3 AC = C - A;
    const Float3 AP = p - A;

    const float ABoAP = AB.dot(AP);
    const float ACoAP = AC.dot(AP);
    if (ABoAP <= 0.0f && ACoAP <= 0.0f)
    {
        return (p - A).lengthSq(); // A 頂点
    }

    const Float3 BP = p - B;
    const float ABoBP = AB.dot(BP);
    const float BAoBP = -ABoBP;
    const float ACoBP = AC.dot(BP);
    const float BCoBP = ACoBP - ABoBP;
    if (BAoBP <= 0.0f && BCoBP <= 0.0f)
    {
        return (p - B).lengthSq(); // B 頂点
    }

    const float vc = ABoAP * ACoBP - ABoBP * ACoAP;
    if (vc <= 0.0f && ABoAP >= 0.0f && ABoBP <= 0.0f)
    {
        float v = ABoAP / (ABoAP - ABoBP);
        Float3 proj = A + AB * v;
        return (p - proj).lengthSq(); // AB 辺上
    }

    const Float3 CP = p - C;
    const float ACoCP = AC.dot(CP);
    const float CAoCP = -ACoCP;
    const float ABoCP = AB.dot(CP);
    const float CBoCP = ABoCP - ACoCP;
    if (CAoCP <= 0.0f && CBoCP <= 0.0f)
    {
        return (p - C).lengthSq(); // C 頂点
    }

    const float vb = ABoCP * ACoAP - ABoAP * ACoCP;
    if (vb <= 0.0f && ACoAP >= 0.0f && ACoCP <= 0.0f)
    {
        float w = ACoAP / (ACoAP - ACoCP);
        Float3 proj = A + AC * w;
        return (p - proj).lengthSq(); // AC 辺上
    }

    const float va = ABoBP * ACoCP - ABoCP * ACoBP;
    if (va <= 0.0f && (ACoBP - ABoBP) >= 0.0f && (ABoCP - ACoCP) >= 0.0f)
    {
        float w = (ACoBP - ABoBP) / ((ACoBP - ABoBP) + (ABoCP - ACoCP));
        Float3 proj = B + (C - B) * w;
        return (p - proj).lengthSq(); // BC 辺上
    }

    // 面内部
    Float3 N = AB.cross(AC);
    const float APoN = AP.dot(N);
    return APoN * APoN / std::max(EPS_ZERO, N.lengthSq());

    // const float dist = AP.dot(N) / std::sqrt(std::max(EPS_ZERO, N.lengthSq()));
    // return dist * dist;
}

float TY::DistanceSq(const Float3& p, const Quad3D& quad)
{
    // TODO: 本当に最適化をする
    // A, B, C, D, AB, BC, CD, DA を順番にチェックするほうが良いと思う

    Triangle3D t1{quad.p0, quad.p1, quad.p2};
    Triangle3D t2{quad.p0, quad.p2, quad.p3};

    return std::min(DistanceSq(p, t1), DistanceSq(p, t2));
}

bool TY::Intersects(const Aabb3D& lhs, const Aabb3D& rhs)
{
    if (lhs.max.x < rhs.min.x || lhs.min.x > rhs.max.x) return false;
    if (lhs.max.y < rhs.min.y || lhs.min.y > rhs.max.y) return false;
    if (lhs.max.z < rhs.min.z || lhs.min.z > rhs.max.z) return false;
    return true;
}

std::optional<Float3> TY::IntersectsAt(const Line3D& line, const Plane3D& plane, float* hitDistance)
{
    const Float3& p0 = line.point;
    const Float3& dir = line.normalizedDir;
    const Float3& n = plane.normal;
    const float d = plane.d;

    const float denom = n.dot(dir);
    if (std::abs(denom) < EPS_PARALLEL)
    {
        return std::nullopt; // 平行
    }

    const float t = -(n.dot(p0) + d) / denom;
    if (hitDistance)
    {
        *hitDistance = t;
    }

    return p0 + dir * t;
}

bool TY::Intersects(const LineSegment3D& segment, const Triangle3D& tri)
{
    return intersectsInternal(segment, tri, nullptr);
}

bool TY::Intersects(const LineSegment3D& segment, const Quad3D& quad)
{
    // TODO: 本当に最適化をする

    Triangle3D t1{quad.p0, quad.p1, quad.p2};
    Triangle3D t2{quad.p0, quad.p2, quad.p3};

    return Intersects(segment, t1) || Intersects(segment, t2);
}

std::optional<Float3> TY::IntersectsAt(const LineSegment3D& segment, const Plane3D& plane)
{
    const Float3 dir = segment.p1 - segment.p0;
    const float denom = plane.normal.dot(dir);
    if (std::abs(denom) < 1e-6f)
    {
        // 平行
        return std::nullopt;
    }

    const float t = -(plane.normal.dot(segment.p0) + plane.d) / denom;
    if (t < 0.0f || t > 1.0f)
    {
        return std::nullopt;
    }

    return segment.p0 + dir * t;
}

std::optional<Float3> TY::IntersectsAt(const LineSegment3D& segment, const Triangle3D& tri)
{
    Float3 intersection;
    if (intersectsInternal(segment, tri, &intersection))
    {
        return intersection;
    }
    else
    {
        return std::nullopt;
    }
}

float TY::DistanceSq(const LineSegment3D& lhs, const LineSegment3D& rhs)
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

    if (std::abs(denom) < EPS_PARALLEL)
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

    sc = (std::abs(sD) < EPS_PARALLEL ? 0.0f : sN / sD);
    tc = (std::abs(tD) < EPS_PARALLEL ? 0.0f : tN / tD);

    Float3 dP = w + (u * sc) - (v * tc);
    return dP.lengthSq();
}

float TY::DistanceSq(const LineSegment3D& segment, const Triangle3D& tri)
{
    if (Intersects(segment, tri))
    {
        return 0.0f;
    }

    float best = FLT_MAX;
    best = Min(best, DistanceSq(segment.p0, tri));
    best = Min(best, DistanceSq(segment.p1, tri));

    LineSegment3D e0{tri.p0, tri.p1};
    LineSegment3D e1{tri.p1, tri.p2};
    LineSegment3D e2{tri.p2, tri.p0};

    best = Min(best, DistanceSq(segment, e0));
    best = Min(best, DistanceSq(segment, e1));
    best = Min(best, DistanceSq(segment, e2));

    return best;
}

float TY::DistanceSq(const LineSegment3D& segment, const Quad3D& quad)
{
    if (Intersects(segment, quad))
    {
        return 0.0f;
    }

    float best = FLT_MAX;
    best = Min(best, DistanceSq(segment.p0, quad));
    best = Min(best, DistanceSq(segment.p1, quad));

    LineSegment3D e0{quad.p0, quad.p1};
    LineSegment3D e1{quad.p1, quad.p2};
    LineSegment3D e2{quad.p2, quad.p3};
    LineSegment3D e3{quad.p3, quad.p0};

    best = Min(best, DistanceSq(segment, e0));
    best = Min(best, DistanceSq(segment, e1));
    best = Min(best, DistanceSq(segment, e2));
    best = Min(best, DistanceSq(segment, e3));

    return best;
}

bool TY::Intersects(const Triangle3D& tri, const LineSegment3D& segment)
{
    return Intersects(segment, tri);
}

bool TY::Intersects(const Triangle3D& tri, const Capsule& capsule)
{
    const float r2 = capsule.radius * capsule.radius;
    const float dist2 = DistanceSq(LineSegment3D{capsule.p0, capsule.p1}, tri);
    return dist2 <= r2;
}

bool TY::Intersects(const Quad3D& quad, const Capsule& capsule)
{
    const float r2 = capsule.radius * capsule.radius;
    const float dist2 = DistanceSq(LineSegment3D{capsule.p0, capsule.p1}, quad);
    return dist2 <= r2;
}

bool TY::Intersects(const Capsule& capsule, const Triangle3D& tri)
{
    return Intersects(tri, capsule);
}

bool TY::Intersects(const Capsule& capsule, const Quad3D& quad)
{
    return Intersects(quad, capsule);
}
