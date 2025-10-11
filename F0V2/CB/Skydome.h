#pragma once
#include "TY/Color.h"

inline namespace CB
{
    struct Skydome_b10
    {
        alignas(16) ColorF32 topColor;
        alignas(16) ColorF32 bottomColor;
        float sphereRadius{};
    };
}
