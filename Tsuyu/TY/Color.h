#pragma once

namespace TY
{
    struct ColorF32
    {
        float r;
        float g;
        float b;
        float a;

        float* getPointer() { return &r; }
    };

    struct ColorU8
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;

        ColorU8() = default;

        ColorU8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) :
            r(r),
            g(g),
            b(b),
            a(a)
        {
        }

        ColorU8(uint8_t color, uint8_t alpha = 255) :
            r(color),
            g(color),
            b(color),
            a(alpha)
        {
        }

        uint8_t* getPointer() { return &r; }
    };
}
