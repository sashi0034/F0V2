#pragma once

#include "Concepts.h"

namespace TY
{
    template <typename T>
    constexpr T Min(T a, T b) noexcept
    {
        return (a < b) ? a : b;
    }

    template <typename T>
    constexpr T Max(T a, T b) noexcept
    {
        return (a > b) ? a : b;
    }

    template <typename T>
    constexpr T Abs(T value) noexcept
    {
        return (value < 0) ? -value : value;
    }

    template <typename T>
    constexpr bool InRange(T value, T min, T max) noexcept
    {
        return min <= value && value <= max;
    }

    template <typename T>
    constexpr T MinVector2(T a, T b) noexcept
    {
        T result;
        result.x = Min(a.x, b.x);
        result.y = Min(a.y, b.y);
        return result;
    }

    template <typename T>
    constexpr T MaxVector2(T a, T b) noexcept
    {
        T result;
        result.x = Max(a.x, b.x);
        result.y = Max(a.y, b.y);
        return result;
    }

    template <typename T>
    constexpr T MinVector3(T a, T b) noexcept
    {
        T result;
        result.x = Min(a.x, b.x);
        result.y = Min(a.y, b.y);
        result.z = Min(a.z, b.z);
        return result;
    }

    template <typename T>
    constexpr T MaxVector3(T a, T b) noexcept
    {
        T result;
        result.x = Max(a.x, b.x);
        result.y = Max(a.y, b.y);
        result.z = Max(a.z, b.z);
        return result;
    }

    namespace Math
    {
        template <FloatingPoint Float>
        inline constexpr Float Pi_v = Float(3.141592653589793238462643383279502884L);

        /// @brief π
        inline constexpr double Pi = Pi_v<double>;

        /// @brief π
        inline constexpr float PiF = Pi_v<float>;

        inline constexpr double HalfPi = Pi_v<double> / 2.0;

        inline constexpr float HalfPiF = Pi_v<float> / 2.0f;

        constexpr auto ToDegrees(Arithmetic auto rad) noexcept -> decltype(rad)
        {
            return rad * 180 / Pi_v<decltype(rad)>;
        }

        constexpr auto ToRadians(Arithmetic auto deg) noexcept -> decltype(deg)
        {
            return deg * Pi_v<decltype(deg)> / 180;
        }

        constexpr double Clamp(double value, double min, double max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }

        constexpr float Clamp(float value, float min, float max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }

        template <typename T, typename U, typename V>
        constexpr T Clamp(T value, U min, V max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }

        constexpr double Lerp(double v1, double v2, double f) noexcept
        {
            return (v1 + (v2 - v1) * f);
        }

        constexpr float Lerp(float v1, float v2, float f) noexcept
        {
            return (v1 + (v2 - v1) * f);
        }

        float Fraction(float x) noexcept;

        double Fraction(double x) noexcept;

        template <Arithmetic Arithmetic>
        inline constexpr int Sign(const Arithmetic x) noexcept
        {
            if (x < 0)
            {
                return -1;
            }
            else if (0 < x)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        float Mod(float a, float b);

        double Mod(double a, double b);
    }

    inline namespace Literals
    {
        constexpr double operator ""_deg(const long double deg) noexcept
        {
            return static_cast<double>(deg * Math::Pi_v<long double> / 180);
        }

        constexpr double operator ""_deg(const unsigned long long deg) noexcept
        {
            return static_cast<double>(deg * Math::Pi_v<long double> / 180);
        }
    }
}
