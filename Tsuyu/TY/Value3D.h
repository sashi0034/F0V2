#pragma once
#include "TemplateHelper.h"

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

        constexpr Vector3D(value_type value) : x(value), y(value), z(value)
        {
        }

        constexpr Vector3D(value_type x, value_type y, value_type z) : x(x), y(y), z(z)
        {
        }

        template <typename OtherType> requires std::is_convertible_v<OtherType, value_type>
        [[nodiscard]] Vector3D(const Vector3D<OtherType>& rhs) : x(rhs.x), y(rhs.y), z(rhs.z)
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

        [[nodiscard]] value_type length() const
        {
            return std::sqrt(this->dot(*this));
        }

        [[nodiscard]] constexpr Vector3D normalized() const
        {
            return *this / length();
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
    };

    using Vec3 = Vector3D<double>;

    using Float3 = Vector3D<float>;

    template <typename T>
    concept FloatingPoint3D = std::is_base_of_v<Vector3D<T>, T>;
}
