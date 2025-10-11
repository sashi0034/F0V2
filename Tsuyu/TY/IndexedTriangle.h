#pragma once
#include "PrimitiveTypes3D.h"

namespace TY
{
    struct IndexedTriangle : Triangle3D
    {
        uint64_t id;

        IndexedTriangle() = default;

        IndexedTriangle(const Float3& p0, const Float3& p1, const Float3& p2, uint64_t id)
            : Triangle3D{p0, p1, p2}, id{id}
        {
        }

        IndexedTriangle(const Triangle3D& tri, uint64_t id)
            : Triangle3D{tri}, id{id}
        {
        }
    };
}
