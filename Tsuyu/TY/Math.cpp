#include "pch.h"
#include "Math.h"

namespace TY
{
    float Math::Fraction(float x) noexcept
    {
        return (x - std::floor(x));
    }

    double Math::Fraction(double x) noexcept
    {
        return (x - std::floor(x));
    }

    float Math::Mod(float a, float b)
    {
        return std::fmod(std::fmod(a, b) + b, b);
    }

    double Math::Mod(double a, double b)
    {
        return std::fmod(std::fmod(a, b) + b, b);
    }
}
