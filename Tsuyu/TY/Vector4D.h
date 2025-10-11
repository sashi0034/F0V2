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

        template <class OtherType> requires std::is_floating_point_v<OtherType>
        [[nodiscard]] constexpr Vector4D<OtherType> cast() const noexcept
        {
            return {
                static_cast<OtherType>(x),
                static_cast<OtherType>(y),
                static_cast<OtherType>(z),
                static_cast<OtherType>(w)
            };
        }

        template <class OtherType>
        [[nodiscard]] constexpr OtherType cast() const noexcept
        {
            OtherType other{};
            other.x = x;
            other.y = y;
            other.z = z;
            other.w = w;
            return other;
        }
    };

    using Float4 = Vector4D<float>;
}
