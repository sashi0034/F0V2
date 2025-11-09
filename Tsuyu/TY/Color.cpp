#include "pch.h"
#include "Color.h"

#include "Math.h"

namespace
{
    double sRGBToLinear(double x)
    {
        return ((x < 0.04045) ? (x / 12.92) : std::pow((x + 0.055) / 1.055, 2.4));
    }

    double linearToSRGB(double x)
    {
        return ((x < 0.0031308) ? (12.92 * x) : (1.055 * std::pow(x, (1.0 / 2.4)) - 0.055));
    }
}

namespace TY
{
    ColorF32 ColorF32::lerp(const ColorF32& other, float rate) const
    {
        return ColorF32{
            Math::Clamp(r + (other.r - r) * rate, 0.0f, 1.0f),
            Math::Clamp(g + (other.g - g) * rate, 0.0f, 1.0f),
            Math::Clamp(b + (other.b - b) * rate, 0.0f, 1.0f),
            Math::Clamp(a + (other.a - a) * rate, 0.0f, 1.0f)
        };
    }

    ColorF32 ColorF32::linearToSRGB() const
    {
        return ColorF32{
            static_cast<float>(::linearToSRGB(static_cast<double>(r))),
            static_cast<float>(::linearToSRGB(static_cast<double>(g))),
            static_cast<float>(::linearToSRGB(static_cast<double>(b))),
            a
        };
    }

    ColorF32 ColorF32::sRGBToLinear() const
    {
        return ColorF32{
            static_cast<float>(::sRGBToLinear(static_cast<double>(r))),
            static_cast<float>(::sRGBToLinear(static_cast<double>(g))),
            static_cast<float>(::sRGBToLinear(static_cast<double>(b))),
            a
        };
    }

    ColorF32 ColorF32::operator*(float factor) const
    {
        return ColorF32{
            Math::Clamp(r * factor, 0.0f, 1.0f),
            Math::Clamp(g * factor, 0.0f, 1.0f),
            Math::Clamp(b * factor, 0.0f, 1.0f),
            Math::Clamp(a * factor, 0.0f, 1.0f)
        };
    }

    ColorF32 sRGB(float r, float g, float b)
    {
        return ColorF32{
            static_cast<float>(::sRGBToLinear(static_cast<double>(r))),
            static_cast<float>(::sRGBToLinear(static_cast<double>(g))),
            static_cast<float>(::sRGBToLinear(static_cast<double>(b))),
            1.0f
        };
    }

    ColorF32 sRGB(Float3 rgb)
    {
        return ColorF32{
            static_cast<float>(::sRGBToLinear(static_cast<double>(rgb.x))),
            static_cast<float>(::sRGBToLinear(static_cast<double>(rgb.y))),
            static_cast<float>(::sRGBToLinear(static_cast<double>(rgb.z))),
            1.0f
        };
    }

    ColorU8 ColorU8::multiplied(float rgbFactor, float alphaFactor) const
    {
        return ColorU8{
            static_cast<uint8_t>(Math::Clamp(static_cast<float>(r) * rgbFactor, 0.0f, 255.0f)),
            static_cast<uint8_t>(Math::Clamp(static_cast<float>(g) * rgbFactor, 0.0f, 255.0f)),
            static_cast<uint8_t>(Math::Clamp(static_cast<float>(b) * rgbFactor, 0.0f, 255.0f)),
            static_cast<uint8_t>(Math::Clamp(static_cast<float>(a) * alphaFactor, 0.0f, 255.0f))
        };
    }

    ColorF32 UnifiedColor::toColorF32() const
    {
        if (std::holds_alternative<ColorF32>(value))
        {
            return std::get<ColorF32>(value);
        }
        else
        {
            return std::get<ColorU8>(value).toColorF32();
        }
    }

    ColorU8 UnifiedColor::toColorU8() const
    {
        if (std::holds_alternative<ColorU8>(value))
        {
            return std::get<ColorU8>(value);
        }
        else
        {
            return std::get<ColorF32>(value).toColorU8();
        }
    }

    UnifiedColor::operator ColorF32() const
    {
        return toColorF32();
    }

    UnifiedColor::operator ColorU8() const
    {
        return toColorU8();
    }
}
