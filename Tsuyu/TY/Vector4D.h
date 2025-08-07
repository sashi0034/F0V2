#pragma once

namespace TY
{
    template <class Type>
    struct Vector4D
    {
        using value_type = Type;

        value_type x;

        value_type y;

        value_type z;

        value_type w;

        constexpr Vector4D() noexcept
            : x{}, y{}, z{}, w{}
        {
        }

        constexpr Vector4D(value_type x_, value_type y_, value_type z_, value_type w_) noexcept
            : x{x_}, y{y_}, z{z_}, w{w_}
        {
        }

        constexpr Vector4D(value_type value) noexcept
            : x{value}, y{value}, z{value}, w{value}
        {
        }
    };

    using Float4 = Vector4D<float>;
}
