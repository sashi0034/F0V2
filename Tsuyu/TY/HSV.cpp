// imported from Siv3D/src/Siv3D/HSV/SivHSV.cpp under the MIT License

#include "pch.h"
#include "HSV.h"

#include "Math.h"

using namespace TY;

namespace
{
    [[nodiscard]]
    inline static double Fraction(const double x) noexcept
    {
        return (x - std::floor(x));
    }

    inline constexpr uint8_t HSVConversionTable[6][3] =
    {
        {3, 2, 0},
        {1, 3, 0},
        {0, 3, 2},
        {0, 1, 3},
        {2, 0, 3},
        {3, 0, 1},
    };

    //
    // http://lol.zoy.org/blog/2013/01/13/fast-rgb-to-hsv
    //
    [[nodiscard]]
    inline HSV RGBAToHSV(double r, double g, double b, double a) noexcept
    {
        double K = 0.0;

        if (g < b)
        {
            std::swap(g, b);
            K = -360.0;
        }

        if (r < g)
        {
            std::swap(r, g);
            K = -720.0 / 6.0 - K;
        }

        const double delta = (g - b) * (360.0 / 6.0);
        const double chroma = r - Min(g, b);
        return HSV(Abs(K + delta / (chroma + 1e-20)), chroma / (r + 1e-20), r, a);
    }
}

namespace TY
{
    HSV::HSV(const ColorU8& color)
        : HSV(RGBAToHSV(color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0))
    {
    }

    HSV::HSV(const ColorF32& color)
        : HSV(color.toColorU8())
    {
    }

    ColorF32 HSV::toColorF() const
    {
        const double hue01 = Fraction(h / 360.0);
        const double hueF = (hue01 * 6.0);
        const int32_t hueI = static_cast<int32_t>(hueF);
        const double fr = (hueF - hueI);
        const double vals[4] =
        {
            v * (1.0 - s),
            v * (1.0 - s * fr),
            v * (1.0 - s * (1.0 - fr)),
            v
        };

        return ColorF32{
            vals[HSVConversionTable[hueI][0]],
            vals[HSVConversionTable[hueI][1]],
            vals[HSVConversionTable[hueI][2]],
            alpha
        };
    }
}
