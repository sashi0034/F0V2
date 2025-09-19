#include "pch.h"
#include "Color.h"

#include "Math.h"

namespace TY
{
    ColorF32 ColorF32::operator*(float factor) const
    {
        return ColorF32{
            Math::Clamp(r * factor, 0.0f, 1.0f),
            Math::Clamp(g * factor, 0.0f, 1.0f),
            Math::Clamp(b * factor, 0.0f, 1.0f),
            Math::Clamp(a * factor, 0.0f, 1.0f)
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
