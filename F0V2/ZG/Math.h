#pragma once

#include "Concepts.h"

namespace ZG
{
    namespace Math
    {
        template <FloatingPoint Float>
        inline constexpr Float Pi_v = Float(3.141592653589793238462643383279502884L);

        constexpr auto ToDegrees(Arithmetic auto rad) noexcept -> decltype(rad)
        {
            return rad * 180 / Pi_v<decltype(rad)>;
        }

        constexpr auto ToRadians(Arithmetic auto deg) noexcept -> decltype(deg)
        {
            return deg * Pi_v<decltype(deg)> / 180;
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
