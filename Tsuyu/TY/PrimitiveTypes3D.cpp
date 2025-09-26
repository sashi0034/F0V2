#include "pch.h"
#include "PrimitiveTypes3D.h"

namespace
{
    constexpr float EPS = 1e-6f;
}

namespace TY
{
    Float3 Triangle3D::getAreaNormal() const
    {
        const Float3 u = p1 - p0;
        const Float3 v = p2 - p0;
        return u.cross(v);
    }

    Float3 Triangle3D::getNormal() const
    {
        return getAreaNormal().normalized();
    }

    Float3 Triangle3D::centroid() const
    {
        return (p0 + p1 + p2) / 3.0f;
    }

    Plane3D Triangle3D::asPlane() const
    {
        const Float3 normal = getNormal();
        const float d = -normal.dot(p0);
        return Plane3D{normal, d};
    }

    Plane3Points Triangle3D::asPlane3Points() const
    {
        return Plane3Points{p0, p1, p2};
    }

    float Plane3D::signedDistanceFrom(const Float3& p) const
    {
        return normal.dot(p) + d;
    }

    float Plane3D::distanceFrom(const Float3& p) const
    {
        return Abs(signedDistanceFrom(p));
    }

    Float3 Plane3D::projection(const Float3& p) const
    {
        const float dist = signedDistanceFrom(p);
        return p - normal * dist;
    }

    Aabb3D Aabb3D::stretched(float length) const
    {
        const Float3 v{length, length, length};
        return Aabb3D{min - v, max + v};
    }

    Float3 Plane3Points::getNormal() const
    {
        const Float3 u = p1 - p0;
        const Float3 v = p2 - p0;
        return u.cross(v);
    }

    bool Quad3D::isPlanar() const
    {
        // 4点が同一平面上にあるかどうかを確認
        const Float3 normal = (p1 - p0).cross(p2 - p0);
        const float areaScale = normal.length(); // 三角形面積の二倍に相当
        if (areaScale < EPS) return false;
        return std::abs(normal.dot(p3 - p0)) < EPS * areaScale;
    }

    bool Quad3D::isValid() const
    {
        // 4点が同一平面上にあるかどうかを確認
        const Float3 n = (p1 - p0).cross(p2 - p0);

        const float areaScale = n.length(); // 三角形面積の二倍に相当
        if (areaScale < EPS) return false;

        const bool isPlaner = std::abs(n.dot(p3 - p0)) < EPS * areaScale;
        if (not isPlaner) return false;

        // 凸かどうか（p0 --> p1 --> p2 --> p3 --> p0 の順)
        const Float3 e0 = p1 - p0;
        const Float3 e1 = p2 - p1;
        const Float3 e2 = p3 - p2;
        const Float3 e3 = p0 - p3;

        const float s0 = n.dot(e0.cross(e1));
        const float s1 = n.dot(e1.cross(e2));
        const float s2 = n.dot(e2.cross(e3));
        const float s3 = n.dot(e3.cross(e0));

        const float tol = EPS * areaScale; // tolerance: スケール依存

        // すべての符号が一致し、かつ十分に非ゼロであること
        const bool allPositive = (s0 > tol) && (s1 > tol) && (s2 > tol) && (s3 > tol);
        const bool allNegative = (s0 < -tol) && (s1 < -tol) && (s2 < -tol) && (s3 < -tol);

        return allPositive || allNegative;
    }

    Float3 Quad3D::getNormal() const
    {
        return (p1 - p0).cross(p2 - p0).normalized();
    }

    Float3 Quad3D::arithmeticCenter() const
    {
        return (p0 + p1 + p2 + p3) * 0.25f;
    }

    Line3D::Line3D(const Float3& point, const Float3& normalizedDir)
        : point(point), normalizedDir(normalizedDir)
    {
    }

    Line3D Line3D::FromPoints(const Float3& from, const Float3& to)
    {
        const Float3 dir = (to - from).normalized();
        return Line3D{from, dir};
    }

    Aabb3D LineSegment3D::aabb() const
    {
        Float3 min, max;
        if (p0.x < p1.x)
        {
            min.x = p0.x;
            max.x = p1.x;
        }
        else
        {
            min.x = p1.x;
            max.x = p0.x;
        }

        if (p0.y < p1.y)
        {
            min.y = p0.y;
            max.y = p1.y;
        }
        else
        {
            min.y = p1.y;
            max.y = p0.y;
        }

        if (p0.z < p1.z)
        {
            min.z = p0.z;
            max.z = p1.z;
        }
        else
        {
            min.z = p1.z;
            max.z = p0.z;
        }

        return Aabb3D{min, max};
    }

    Aabb3D Capsule::aabb() const
    {
        return LineSegment3D{p0, p1}.aabb().stretched(radius);
    }

    Capsule Capsule::AlongY(const Float3& center, float height, float radius)
    {
        const auto p1 = center + Float3(0, -height * 0.5, 0);
        const auto p2 = center + Float3(0, height * 0.5, 0);
        return Capsule{p1, p2, radius};
    }
}
