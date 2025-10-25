#pragma once
#include "DirectXMath.h"
#include "Vector2D.h"

namespace TY

{
    template <class Type>
    struct Vector3D
    {
        using value_type = Type;
        // using value_type = double;

        value_type x;

        value_type y;

        value_type z;

        static constexpr bool isFloat3 = std::is_same_v<value_type, float>;

        static constexpr bool isVec3 = std::is_same_v<value_type, double>;

        constexpr Vector3D() = default;

        explicit constexpr Vector3D(value_type value) : x(value), y(value), z(value)
        {
        }

        constexpr Vector3D(value_type x, value_type y, value_type z) : x(x), y(y), z(z)
        {
        }

        template <typename FloatType> requires std::is_floating_point_v<FloatType>
        constexpr Vector3D(FloatType x, FloatType y, FloatType z) : x(x), y(y), z(z)
        {
        }

        template <typename OtherType> requires std::is_convertible_v<OtherType, value_type>
        constexpr Vector3D(const Vector3D<OtherType>& rhs) : x(rhs.x), y(rhs.y), z(rhs.z)
        {
        }

        Vector3D(DirectX::XMFLOAT3 xmf) : x(xmf.x), y(xmf.y), z(xmf.z)
        {
        }

        Vector3D(DirectX::XMVECTOR xmv)
        {
            DirectX::XMFLOAT3 tmp;
            XMStoreFloat3(&tmp, xmv);
            *this = tmp;
        }

        [[nodiscard]] constexpr Vector2D<value_type> xy() const
        {
            return Vector2D<value_type>(x, y);
        }

        [[nodiscard]] constexpr Vector2D<value_type> yz() const
        {
            return Vector2D<value_type>(y, z);
        }

        [[nodiscard]] constexpr Vector2D<value_type> xz() const
        {
            return Vector2D<value_type>(x, z);
        }

        [[nodiscard]] constexpr Vector2D<value_type> yx() const
        {
            return Vector2D<value_type>(y, x);
        }

        [[nodiscard]] constexpr Vector2D<value_type> zy() const
        {
            return Vector2D<value_type>(z, y);
        }

        [[nodiscard]] constexpr Vector2D<value_type> zx() const
        {
            return Vector2D<value_type>(z, x);
        }

        [[nodiscard]] constexpr Vector3D operator+() const
        {
            return *this;
        }

        [[nodiscard]] constexpr Vector3D operator-() const
        {
            return Vector3D(-x, -y, -z);
        }

        [[nodiscard]] constexpr Vector3D operator+(const Vector3D& rhs) const
        {
            return Vector3D(x + rhs.x, y + rhs.y, z + rhs.z);
        }

        [[nodiscard]] constexpr Vector3D operator-(const Vector3D& rhs) const
        {
            return Vector3D(x - rhs.x, y - rhs.y, z - rhs.z);
        }

        [[nodiscard]] constexpr Vector3D operator*(value_type rhs) const
        {
            return Vector3D(x * rhs, y * rhs, z * rhs);
        }

        [[nodiscard]] constexpr Vector3D operator*(const Vector3D& rhs) const
        {
            return Vector3D(x * rhs.x, y * rhs.y, z * rhs.z);
        }

        [[nodiscard]] constexpr Vector3D operator/(value_type rhs) const
        {
            return Vector3D(x / rhs, y / rhs, z / rhs);
        }

        Vector3D& operator+=(const Vector3D& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }

        Vector3D& operator-=(const Vector3D& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }

        Vector3D& operator*=(value_type rhs)
        {
            x *= rhs;
            y *= rhs;
            z *= rhs;
            return *this;
        }

        Vector3D& operator/=(value_type rhs)
        {
            x /= rhs;
            y /= rhs;
            z /= rhs;
            return *this;
        }

        [[nodiscard]]
        constexpr value_type elem(size_t index) const
        {
            if (index == 0) return x;
            if (index == 1) return y;
            if (index == 2) return z;
            return 0;
        }

        [[nodiscard]] constexpr Vector3D withX(value_type newX) const
        {
            return Vector3D(newX, y, z);
        }

        [[nodiscard]] constexpr Vector3D withY(value_type newY) const
        {
            return Vector3D(x, newY, z);
        }

        [[nodiscard]] constexpr Vector3D withZ(value_type newZ) const
        {
            return Vector3D(x, y, newZ);
        }

        [[nodiscard]] constexpr bool operator==(const Vector3D& rhs) const
        {
            return x == rhs.x && y == rhs.y && z == rhs.z;
        }

        [[nodiscard]] constexpr bool operator!=(const Vector3D& rhs) const
        {
            return !(*this == rhs);
        }

        bool isZero() const
        {
            return x == 0 && y == 0 && z == 0;
        }

        [[nodiscard]] constexpr value_type dot(const Vector3D& rhs) const
        {
            return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
        }

        [[nodiscard]] constexpr Vector3D cross(const Vector3D& rhs) const
        {
            return Vector3D(
                this->y * rhs.z - this->z * rhs.y,
                this->z * rhs.x - this->x * rhs.z,
                this->x * rhs.y - this->y * rhs.x);
        }

        [[nodiscard]] Vector3D slerp(const Vector3D& target, value_type t) const
        {
            float dot = this->dot(target);
            dot = std::clamp(dot, -1.0f, 1.0f);
            float theta = std::acos(dot) * t;
            Vector3D relative = (target - (*this) * dot).normalized();
            return (*this) * std::cos(theta) + relative * std::sin(theta);
        }

        [[nodiscard]] value_type lengthSq() const
        {
            return this->dot(*this);
        }

        [[nodiscard]] value_type length() const
        {
            return std::sqrt(this->dot(*this));
        }

        [[nodiscard]] Vector3D normalized() const
        {
            const auto length = this->length();
            return length == 0 ? Vector3D{} : *this / length;
        }

        [[nodiscard]] DirectX::XMFLOAT3 toXMF() const
        {
            if constexpr (std::is_same_v<value_type, float>) return {x, y, z};
            else return {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
        }

        [[nodiscard]] DirectX::XMVECTOR toXMV() const
        {
            const auto tmp = toXMF();
            return DirectX::XMLoadFloat3(&tmp);
        }

        [[nodiscard]]
        static Vector3D Zero()
        {
            return Vector3D(0, 0, 0);
        }

        [[nodiscard]]
        static Vector3D One()
        {
            return Vector3D(1, 1, 1);
        }
    };

    using Double3 = Vector3D<double>;

    using Float3 = Vector3D<float>;

    template <typename T>
    concept FloatingPoint3D = std::is_base_of_v<Vector3D<T>, T>;
}

// -----------------------------------------------

template <typename T>
struct std::formatter<TY::Vector3D<T>>
{
    char presentation = 'p';
    std::string elem_fmt;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto it = ctx.begin();

        if (it != ctx.end() && (*it == 'p' || *it == 'b' || *it == 'c' || *it == 'd'))
        {
            presentation = *it++;
        }

        const auto start = it;
        while (it != ctx.end() && *it != '}')
        {
            ++it;
        }

        elem_fmt.assign(start, it);

        return it;
    }

    auto format(const TY::Vector3D<T>& v, std::format_context& ctx) const
    {
        const std::string inner = std::format("{{:{}}}", elem_fmt);

        const std::string fx = std::vformat(inner, std::make_format_args(v.x));
        const std::string fy = std::vformat(inner, std::make_format_args(v.y));
        const std::string fz = std::vformat(inner, std::make_format_args(v.z));

        switch (presentation)
        {
        case 'b': // Brackets
            return std::format_to(ctx.out(), "[{}, {}, {}]", fx, fy, fz);
        case 'c': // Comma
            return std::format_to(ctx.out(), "{}, {}, {}", fx, fy, fz);
        case 'd': // Detailed
            return std::format_to(ctx.out(), "Vector3D(x={}, y={}, z={})", fx, fy, fz);
        case 'p': // Parentheses
        default:
            return std::format_to(ctx.out(), "({}, {}, {})", fx, fy, fz);
        }
    }
};
