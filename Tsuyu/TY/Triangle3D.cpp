#include "pch.h"
#include "Triangle3D.h"

namespace TY
{
    Float3 Triangle3D::getNormal() const
    {
        const Float3 u = p1 - p0;
        const Float3 v = p2 - p0;
        return u.cross(v);
    }
}
