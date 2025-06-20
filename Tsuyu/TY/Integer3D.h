#pragma once

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
    };

    using Point3D = Integer3D<int>;
}
