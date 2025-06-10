#pragma once
#include "Value3D.h"

namespace TY
{
    struct ColorU8;

    struct ColorF32
    {
        float r;
        float g;
        float b;
        float a;

        constexpr ColorF32() = default;

        constexpr ColorF32(float r, float g, float b, float a = 1.0f) :
            r(r),
            g(g),
            b(b),
            a(a)
        {
        }

        constexpr ColorF32(float color, float alpha = 1.0f) :
            r(color),
            g(color),
            b(color),
            a(alpha)
        {
        }

        float* getPointer() { return &r; }

        Float3 toFloat3() const { return Float3{r, g, b}; }

        ColorU8 toColorU8() const;
    };

    struct ColorU8
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;

        constexpr ColorU8() = default;

        constexpr ColorU8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) :
            r(r),
            g(g),
            b(b),
            a(a)
        {
        }

        constexpr ColorU8(uint8_t color, uint8_t alpha = 255) :
            r(color),
            g(color),
            b(color),
            a(alpha)
        {
        }

        uint8_t* getPointer() { return &r; }

        ColorF32 toColorF32() const;
    };

    struct UnifiedColor
    {
        using variant_type = std::variant<ColorF32, ColorU8>;

        variant_type value;

        UnifiedColor() = default;

        UnifiedColor(const ColorF32& color) : value(color)
        {
        }

        UnifiedColor(const ColorU8& color) : value(color)
        {
        }

        ColorF32 toColorF32() const;

        ColorU8 toColorU8() const;

        operator ColorF32() const;

        operator ColorU8() const;
    };
}
