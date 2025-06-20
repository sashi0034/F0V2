#pragma once

namespace TY
{
    template <class Type>
    struct Integer2D
    {
        using value_type = Type;
        // using value_type = double;

        value_type x;

        value_type y;

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
            return {x * s, y * s};
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

        [[nodiscard]] constexpr Integer2D operator %(value_type s) const noexcept
        {
            return {x % s, y % s};
        }

        [[nodiscard]] constexpr Integer2D withX(value_type newX) const noexcept
        {
            return {newX, y};
        }

        [[nodiscard]] constexpr Integer2D withY(value_type newY) const noexcept
        {
            return {x, newY};
        }

        [[nodiscard]] bool isZero() const noexcept
        {
            return x == 0 && y == 0;
        }

        template <class OtherType>
        [[nodiscard]] constexpr Integer2D<OtherType> cast() const noexcept
        {
            return {static_cast<OtherType>(x), static_cast<OtherType>(y)};
        }

        template <typename T = float>
        [[nodiscard]] constexpr T horizontalAspectRatio() const noexcept
        {
            return static_cast<T>(x) / static_cast<T>(y);
        }
    };

    using Point = Integer2D<int32_t>;

    using Size = Point;
}
