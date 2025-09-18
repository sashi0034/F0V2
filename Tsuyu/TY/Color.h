#pragma once
#include "Vector3D.h"
#include "Vector4D.h"

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

        constexpr ColorF32(double r, double g, double b, double a = 1.0f) :
            r(static_cast<double>(r)),
            g(static_cast<double>(g)),
            b(static_cast<double>(b)),
            a(static_cast<double>(a))
        {
        }

        explicit constexpr ColorF32(float color, float alpha = 1.0f) :
            r(color),
            g(color),
            b(color),
            a(alpha)
        {
        }

        explicit constexpr ColorF32(std::string_view code);

        float* getPointer() { return &r; }

        Float3 toFloat3() const { return Float3{r, g, b}; }

        Float4 toFloat4() const { return Float4{r, g, b, a}; }

        constexpr ColorU8 toColorU8() const;
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

        constexpr ColorU8(std::string_view code);

        uint8_t* getPointer() { return &r; }

        constexpr ColorF32 toColorF32() const;

        ColorU8 multiplied(float rgbFactor, float alphaFactor = 1.0f) const;
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

    // -----------------------------------------------

    namespace detail
    {
        constexpr uint32_t HexToDecimal(char c) noexcept
        {
            return (c & 0xF) + ((c & 0x40) >> 6) * 9;
        }
    }

    constexpr ColorF32::ColorF32(std::string_view code)
    {
        *this = ColorU8(code).toColorF32();
    }

    constexpr ColorU8 ColorF32::toColorU8() const
    {
        return ColorU8{
            static_cast<uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f))
        };
    }

    constexpr ColorU8::ColorU8(std::string_view code)
    {
        const char* c = code.data();
        const size_t length = code.length();
        if (6 <= length && length <= 8)
        {
            if (length == 7 || length == 9)
            {
                c++;
            }

            r = static_cast<uint8_t>(detail::HexToDecimal(c[0]) * 16 + detail::HexToDecimal(c[1]));
            g = static_cast<uint8_t>(detail::HexToDecimal(c[2]) * 16 + detail::HexToDecimal(c[3]));
            b = static_cast<uint8_t>(detail::HexToDecimal(c[4]) * 16 + detail::HexToDecimal(c[5]));
            a = length == 8 ? static_cast<uint8_t>(detail::HexToDecimal(c[6]) * 16 + detail::HexToDecimal(c[7])) : 255;
        }
        else
        {
            r = 255;
            g = 0;
            b = 255;
            a = 255;
        }
    }

    constexpr ColorF32 ColorU8::toColorF32() const
    {
        return ColorF32{
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f
        };
    }
}
