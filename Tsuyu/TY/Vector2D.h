#pragma once
#include <type_traits>

#include "Integer2D.h"
#include "TemplateHelper.h"

namespace TY
{
    template <class Type>
    struct Vector2D
    {
        using value_type = Type;
        // using value_type = double;

        value_type x;

        value_type y;

        [[nodiscard]] constexpr Vector2D() = default;

        [[nodiscard]] constexpr Vector2D(value_type _x, value_type _y) noexcept
            : x(_x), y(_y)
        {
        }

        template <typename OtherType>
        [[nodiscard]] constexpr Vector2D(const Vector2D<OtherType>& other) noexcept
            : x(other.x), y(other.y)
        {
        }

        template <typename OtherType>
        [[nodiscard]] constexpr Vector2D(const Integer2D<OtherType>& other) noexcept
            : x(other.x), y(other.y)
        {
        }

        [[nodiscard]] constexpr Vector2D operator +() const noexcept
        {
            return *this;
        }

        [[nodiscard]] constexpr Vector2D operator -() const noexcept
        {
            return {-x, -y};
        }

        [[nodiscard]] constexpr Vector2D operator +(const Vector2D& v) const noexcept
        {
            return {x + v.x, y + v.y};
        }

        [[nodiscard]] constexpr Vector2D operator -(Vector2D v) const noexcept
        {
            return {x - v.x, y - v.y};
        }

        [[nodiscard]] constexpr Vector2D operator *(value_type s) const noexcept
        {
            return {x * s, y * s};
        }

        [[nodiscard]] constexpr Vector2D operator *(Vector2D v) const noexcept
        {
            return {x * v.x, y * v.y};
        }

        [[nodiscard]] constexpr Vector2D operator /(value_type s) const noexcept
        {
            return {x / s, y / s};
        }

        [[nodiscard]] constexpr Vector2D operator /(Vector2D v) const noexcept
        {
            return {x / v.x, y / v.y};
        }

        [[nodiscard]] constexpr Vector2D operator %(value_type s) const noexcept
        {
            return {x % s, y % s};
        }

        [[nodiscard]] constexpr Vector2D withX(value_type newX) const noexcept
        {
            return {newX, y};
        }

        [[nodiscard]] constexpr Vector2D withY(value_type newY) const noexcept
        {
            return {x, newY};
        }

        [[nodiscard]] bool isZero() const noexcept
        {
            return x == 0 && y == 0;
        }

        [[nodiscard]] constexpr Point asPoint() const noexcept
        {
            return {static_cast<int>(x), static_cast<int>(y)};
        }

        template <class OtherType> requires std::is_floating_point_v<OtherType>
        [[nodiscard]] constexpr Vector2D<OtherType> cast() const noexcept
        {
            return {static_cast<OtherType>(x), static_cast<OtherType>(y)};
        }

        template <class OtherType>
        [[nodiscard]] constexpr OtherType cast() const noexcept
        {
            OtherType other{};
            other.x = x;
            other.y = y;
            return other;
        }

        template <typename T = value_type>
        [[nodiscard]] constexpr T horizontalAspectRatio() const noexcept
        {
            return static_cast<T>(x) / static_cast<T>(y);
        }
    };

    using Vec2 = Vector2D<double>;

    using Float2 = Vector2D<float>;
}
