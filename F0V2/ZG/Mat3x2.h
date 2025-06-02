#pragma once
#include "Value2D.h"

namespace ZG
{
    struct Mat3x2
    {
        using value_type = float;

        value_type _11, _12;
        value_type _21, _22;
        value_type _31, _32;

        [[nodiscard]]
        constexpr Float2 transformPoint(const Float2& pos) const
        {
            return {
                _11 * pos.x + _21 * pos.y + _31,
                _12 * pos.x + _22 * pos.y + _32
            };
        }

        static constexpr Mat3x2 Identity()
        {
            return {
                1.0f, 0.0f,
                0.0f, 1.0f,
                0.0f, 0.0f
            };
        }

        static constexpr Mat3x2 Screen(const Float2& size)
        {
            return {
                2.0f / size.x, 0.0f,
                0.0f, -2.0f / size.y,
                -1.0f, 1.0f
            };
        }
    };
}
