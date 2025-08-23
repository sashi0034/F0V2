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

        template <typename ArithmeticType> requires std::is_arithmetic_v<ArithmeticType>
        [[nodiscard]] constexpr Vector2D(ArithmeticType _x, ArithmeticType _y) noexcept
            : x(value_type(_x)), y(value_type(_y))
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

        constexpr Vector2D operator+=(const Vector2D& v) noexcept
        {
            x += v.x;
            y += v.y;
            return *this;
        }

        [[nodiscard]] constexpr Vector2D operator -(Vector2D v) const noexcept
        {
            return {x - v.x, y - v.y};
        }

        constexpr Vector2D operator-=(const Vector2D& v) noexcept
        {
            x -= v.x;
            y -= v.y;
            return *this;
        }

        [[nodiscard]] constexpr Vector2D operator *(value_type s) const noexcept
        {
            return {x * s, y * s};
        }

        constexpr Vector2D operator*=(const Vector2D& v) noexcept
        {
            x *= v.x;
            y *= v.y;
            return *this;
        }

        [[nodiscard]] constexpr Vector2D operator *(Vector2D v) const noexcept
        {
            return {x * v.x, y * v.y};
        }

        constexpr Vector2D operator*=(value_type s) noexcept
        {
            x *= s;
            y *= s;
            return *this;
        }

        [[nodiscard]] friend Vector2D operator*(value_type lhs, const Vector2D& rhs)
        {
            return {lhs * rhs.x, lhs * rhs.y};
        }

        [[nodiscard]] constexpr Vector2D operator /(value_type s) const noexcept
        {
            return {x / s, y / s};
        }

        constexpr Vector2D operator/=(value_type s) noexcept
        {
            x /= s;
            y /= s;
            return *this;
        }

        [[nodiscard]] constexpr Vector2D operator /(Vector2D v) const noexcept
        {
            return {x / v.x, y / v.y};
        }

        constexpr Vector2D operator/=(const Vector2D& v) noexcept
        {
            x /= v.x;
            y /= v.y;
            return *this;
        }

        [[nodiscard]] constexpr Vector2D operator %(value_type s) const noexcept
        {
            return {x % s, y % s};
        }

        [[nodiscard]] constexpr Vector2D operator%=(value_type s) noexcept
        {
            x %= s;
            y %= s;
            return *this;
        }

        [[nodiscard]] constexpr Vector2D withX(value_type newX) const noexcept
        {
            return {newX, y};
        }

        [[nodiscard]] constexpr Vector2D withY(value_type newY) const noexcept
        {
            return {x, newY};
        }

        [[nodiscard]] constexpr value_type maxComponent() const noexcept
        {
            return (x > y) ? x : y;
        }

        [[nodiscard]] constexpr value_type minComponent() const noexcept
        {
            return (x < y) ? x : y;
        }

        [[nodiscard]] value_type length() const noexcept
        {
            return std::sqrt(x * x + y * y);
        }

        [[nodiscard]] value_type lengthSq() const noexcept
        {
            return x * x + y * y;
        }

        [[nodiscard]] Vector2D normalized() const noexcept
        {
            const auto len = length();
            if (len == 0) return {0, 0};
            return {x / len, y / len};
        }

        [[nodiscard]] value_type dot(const Vector2D& v) const noexcept
        {
            return x * v.x + y * v.y;
        }

        [[nodiscard]] value_type cross(const Vector2D& v) const noexcept
        {
            return x * v.y - y * v.x;
        }

        [[nodiscard]] bool isZero() const noexcept
        {
            return x == 0 && y == 0;
        }

        [[nodiscard]] bool inBounds(Vector2D min, Vector2D max) const noexcept
        {
            return (min.x <= x && x <= max.x) && (min.y <= y && y <= max.y);
        }

        template <class MinVector, class MaxVector>
        [[nodiscard]] bool inBounds(MinVector min, MaxVector max) const noexcept
        {
            return (min.x <= x && x <= max.x) && (min.y <= y && y <= max.y);
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

    using SizeF = Float2;
}
