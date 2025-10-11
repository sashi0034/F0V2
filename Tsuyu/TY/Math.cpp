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

    float Math::NormalizeAngle(float radian, float center)
    {
        radian = Math::Mod(radian + (PiF - center), TwoPiF);

        if (radian < 0.0f)
        {
            radian += TwoPiF;
        }

        return (radian - (PiF - center));
    }

    double Math::NormalizeAngle(double radian, const double center)
    {
        radian = Math::Mod(radian + (Pi - center), TwoPi);

        if (radian < 0.0)
        {
            radian += TwoPi;
        }

        return (radian - (Pi - center));
    }
}
