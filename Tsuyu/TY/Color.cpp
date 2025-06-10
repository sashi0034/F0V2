#include "pch.h"
#include "Color.h"

namespace TY
{
    ColorU8 ColorF32::toColorU8() const
    {
        return ColorU8{
            static_cast<uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f))
        };
    }

    ColorF32 ColorU8::toColorF32() const
    {
        return ColorF32{
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f
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
