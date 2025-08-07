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
}
