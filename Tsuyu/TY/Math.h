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

    template <typename T>
    constexpr bool InRange(T value, T min, T max) noexcept
    {
        return min <= value && value <= max;
    }

    template <Integral Integral>
    constexpr Integral Modulo(Integral a, Integral b) noexcept
    {
        return ((a % b) + b) % b;
    }

    namespace Math
    {
        template <FloatingPoint Float>
        inline constexpr Float Pi_v = Float(3.141592653589793238462643383279502884L);

        template <FloatingPoint Float>
        inline constexpr Float TwoPi_v = 2 * Pi_v<Float>;

        /// @brief π
        inline constexpr double Pi = Pi_v<double>;

        /// @brief π
        inline constexpr float PiF = Pi_v<float>;

        inline constexpr double TwoPi = Pi_v<double> * 2.0;

        inline constexpr float TwoPiF = Pi_v<float> * 2.0f;

        inline constexpr double HalfPi = Pi_v<double> / 2.0;

        inline constexpr float HalfPiF = Pi_v<float> / 2.0f;

        inline constexpr double InvHalfPi = 2.0 / Pi_v<double>;

        inline constexpr float InvHalfPiF = 2.0f / Pi_v<float>;

        inline constexpr double InvPi = 1.0 / Pi_v<double>;

        inline constexpr float InvPiF = 1.0f / Pi_v<float>;

        inline constexpr double InvTwoPi = 1.0 / (Pi_v<double> * 2.0);

        inline constexpr float InvTwoPiF = 1.0f / (Pi_v<float> * 2.0f);

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

        template <class T, class U, class V>
        inline auto LerpAngle(const T from, const U to, const V t) noexcept
        {
            const auto diff = std::fmod(to - from, Math::TwoPi_v<V>);
            return (from + (std::fmod(2 * diff, Math::TwoPi_v<V>) - diff) * t);
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

        template <Arithmetic Arithmetic>
        inline constexpr Arithmetic Square(const Arithmetic x) noexcept
        {
            return x * x;
        }

        // template <Integral Integral>
        // constexpr Integral Mod(Integral a, Integral b) noexcept
        // {
        //     return ((a % b) + b) % b;
        // }

        float Mod(float a, float b);

        double Mod(double a, double b);

        float NormalizeAngle(float radian, float center);

        inline double NormalizeAngle(double radian, double center);
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
