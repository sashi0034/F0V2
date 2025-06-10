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

    namespace Math
    {
        template <FloatingPoint Float>
        inline constexpr Float Pi_v = Float(3.141592653589793238462643383279502884L);

        /// @brief π
        inline constexpr double Pi = Pi_v<double>;

        /// @brief π
        inline constexpr float PiF = Pi_v<float>;

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

        constexpr double Clamp(float value, float min, float max)
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
