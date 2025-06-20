#pragma once
#include "Integer2D.h"

namespace TY
{
    template <class Type>
    struct Integer3D
    {
        using value_type = Type;
        // using value_type = int;

        value_type x;

        value_type y;

        value_type z;

        Integer3D() : x(0), y(0), z(0)
        {
        }

        Integer3D(value_type x, value_type y, value_type z) : x(x), y(y), z(z)
        {
        }

        Integer3D(Integer2D<Type> xy, value_type z) : x(xy.x), y(xy.y), z(z)
        {
        }
    };

    using Point3D = Integer3D<int>;
}
