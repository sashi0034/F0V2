#pragma once
#include "Math.h"

namespace TY
{
    template <class Type>
    struct Integer2D
    {
        using value_type = Type;
        // using value_type = double;

        value_type x;

        value_type y;

        using float_type = typename std::conditional<(sizeof(Type) > 4), double, float>::type;

        [[nodiscard]] constexpr Integer2D() = default;

        [[nodiscard]] constexpr Integer2D(value_type _x, value_type _y) noexcept
            : x(_x), y(_y)
        {
        }

        template <typename OtherType>
        [[nodiscard]] constexpr Integer2D(const Integer2D<OtherType>& other) noexcept
            : x(other.x), y(other.y)
        {
        }

        [[nodiscard]] constexpr Integer2D operator +() const noexcept
        {
            return *this;
        }

        [[nodiscard]] constexpr Integer2D operator -() const noexcept
        {
            return {-x, -y};
        }

        [[nodiscard]] constexpr Integer2D operator +(const Integer2D& v) const noexcept
        {
            return {x + v.x, y + v.y};
        }

        [[nodiscard]] constexpr Integer2D operator -(Integer2D v) const noexcept
        {
            return {x - v.x, y - v.y};
        }

        [[nodiscard]] constexpr Integer2D operator *(value_type s) const noexcept
        {
            return {x * s, y * s};
        }

        template <typename FloatingPoint> requires std::is_floating_point_v<FloatingPoint>
        [[nodiscard]] constexpr Integer2D operator *(FloatingPoint s) const noexcept
        {
            return {value_type(x * s), value_type(y * s)};
        }

        [[nodiscard]] constexpr Integer2D operator *(Integer2D v) const noexcept
        {
            return {x * v.x, y * v.y};
        }

        template <typename VectorType>
        [[nodiscard]] constexpr VectorType operator *(VectorType v) const noexcept
        {
            return {x * v.x, y * v.y};
        }

        [[nodiscard]] constexpr Integer2D operator /(value_type s) const noexcept
        {
            return {x / s, y / s};
        }

        template <typename FloatingPoint> requires std::is_floating_point_v<FloatingPoint>
        [[nodiscard]] constexpr Integer2D operator /(FloatingPoint s) const noexcept
        {
            return {x / s, y / s};
        }

        [[nodiscard]] constexpr Integer2D operator /(Integer2D v) const noexcept
        {
            return {x / v.x, y / v.y};
        }

        [[nodiscard]] constexpr Integer2D operator %(value_type s) const noexcept
        {
            return {x % s, y % s};
        }

        [[nodiscard]] constexpr Integer2D operator %(Integer2D v) const noexcept
        {
            return {x % v.x, y % v.y};
        }

        [[nodiscard]] constexpr bool operator ==(const Integer2D& v) const noexcept
        {
            return x == v.x && y == v.y;
        }

        [[nodiscard]] constexpr bool operator !=(const Integer2D& v) const noexcept
        {
            return !(*this == v);
        }

        [[nodiscard]] constexpr Integer2D& operator +=(const Integer2D& v) noexcept
        {
            x += v.x;
            y += v.y;
            return *this;
        }

        [[nodiscard]] constexpr Integer2D& operator -=(const Integer2D& v) noexcept
        {
            x -= v.x;
            y -= v.y;
            return *this;
        }

        [[nodiscard]] constexpr Integer2D& operator *=(value_type s) noexcept
        {
            x *= s;
            y *= s;
            return *this;
        }

        [[nodiscard]] constexpr Integer2D& operator /=(value_type s) noexcept
        {
            x /= s;
            y /= s;
            return *this;
        }

        [[nodiscard]] constexpr Integer2D withX(value_type newX) const noexcept
        {
            return {newX, y};
        }

        [[nodiscard]] constexpr Integer2D withY(value_type newY) const noexcept
        {
            return {x, newY};
        }

        [[nodiscard]] constexpr Integer2D movedBy(value_type dx, value_type dy) const noexcept
        {
            return {x + dx, y + dy};
        }

        [[nodiscard]] constexpr Integer2D movedBy(const Integer2D& d) const noexcept
        {
            return {x + d.x, y + d.y};
        }

        [[nodiscard]] float_type length() const noexcept
        {
            return std::sqrt(static_cast<float_type>(x) * x + static_cast<float_type>(y) * y);
        }

        [[nodiscard]] bool isZero() const noexcept
        {
            return x == 0 && y == 0;
        }

        [[nodiscard]] int manhattanLength() const noexcept
        {
            return std::abs(x) + std::abs(y);
        }

        [[nodiscard]] value_type maxComponent() const noexcept
        {
            return Max<value_type>(x, y);
        }

        [[nodiscard]] value_type minComponent() const noexcept
        {
            return Min<value_type>(x, y);
        }

        template <class OtherType> requires std::is_integral_v<OtherType>
        [[nodiscard]] constexpr Integer2D<OtherType> cast() const noexcept
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

        /// @brief x / y
        [[nodiscard]] constexpr float_type horizontalAspectRatio() const noexcept
        {
            return static_cast<float_type>(x) / static_cast<float_type>(y);
        }

        static constexpr Integer2D Zero() noexcept
        {
            return {0, 0};
        }

        static constexpr Integer2D One() noexcept
        {
            return {1, 1};
        }
    };

    using Point = Integer2D<int32_t>;

    using Size = Point;
}
