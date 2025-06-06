#pragma once
#include <type_traits>

#include "TemplateHelper.h"

namespace TY
{
    template <class Type>
    struct Value2D
    {
        using value_type = Type;
        // using value_type = double;

        value_type x;

        value_type y;

        static constexpr bool isPoint = std::is_same_v<value_type, int>;

        static constexpr bool isFloat2 = std::is_same_v<value_type, float>;

        static constexpr bool isVec2 = std::is_same_v<value_type, double>;

        [[nodiscard]] constexpr Value2D() = default;

        [[nodiscard]] constexpr Value2D(value_type _x, value_type _y) noexcept
            : x(_x), y(_y)
        {
        }

        template <typename OtherType>
        [[nodiscard]] constexpr Value2D(const Value2D<OtherType>& other) noexcept
            : x(other.x), y(other.y)
        {
        }

        [[nodiscard]] constexpr Value2D operator +() const noexcept
        {
            return *this;
        }

        [[nodiscard]] constexpr Value2D operator -() const noexcept
        {
            return {-x, -y};
        }

        [[nodiscard]] constexpr Value2D operator +(const Value2D& v) const noexcept
        {
            return {x + v.x, y + v.y};
        }

        [[nodiscard]] constexpr Value2D operator -(Value2D v) const noexcept
        {
            return {x - v.x, y - v.y};
        }

        [[nodiscard]] constexpr Value2D operator *(value_type s) const noexcept
        {
            return {x * s, y * s};
        }

        [[nodiscard]] constexpr Value2D operator /(value_type s) const noexcept
        {
            return {x / s, y / s};
        }

        [[nodiscard]] constexpr Value2D withX(value_type newX) const noexcept
        {
            return {newX, y};
        }

        [[nodiscard]] constexpr Value2D withY(value_type newY) const noexcept
        {
            return {x, newY};
        }

        [[nodiscard]] bool isZero() const noexcept
        {
            return x == 0 && y == 0;
        }

        [[nodiscard]] constexpr Value2D<int> toPoint() const noexcept
        {
            return {static_cast<int>(x), static_cast<int>(y)};
        }

        template <class OtherType>
        [[nodiscard]] constexpr Value2D<OtherType> cast() const noexcept
        {
            return {static_cast<OtherType>(x), static_cast<OtherType>(y)};
        }

        template <typename T = std::conditional<std::is_floating_point_v<Type>, Type, double>::type>
        [[nodiscard]] constexpr T horizontalAspectRatio() const noexcept
        {
            return static_cast<T>(x) / static_cast<T>(y);
        }
    };

    /// @brief Floating point 2D vector
    // template <class Type>
    // struct Vector2D : Value2D<Type>
    // {
    //     using Value2D<Type>::Value2D;
    //
    //     [[nodiscard]] constexpr Vector2D(const Value2D<Type>& v)
    //         : Value2D<Type>(v)
    //     {
    //     }
    //
    //     template <class OtherType>
    //     [[nodiscard]] constexpr Vector2D(const Value2D<OtherType>& v)
    //         : Value2D<Type>(static_cast<Type>(v.x), static_cast<Type>(v.y))
    //     {
    //     }
    // };

    using Vec2 = Value2D<double>;

    using Float2 = Value2D<float>;

    /// @brief Integral 2D vector
    // template <class Integer>
    // struct Integer2D : Value2D<Integer>
    // {
    //     using Value2D<Integer>::Value2D;
    // };

    using Point = Value2D<int32_t>;

    using Size = Point;
}
