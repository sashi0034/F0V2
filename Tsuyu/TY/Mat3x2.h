#pragma once
#include "Vector2D.h"

namespace TY
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

        [[nodiscard]]
        Mat3x2 constexpr translated(Float2 v) const noexcept
        {
            Mat3x2 mat = *this;
            mat._31 += v.x;
            mat._32 += v.y;
            return mat;
        }

        [[nodiscard]]
        Mat3x2 constexpr scaled(Float2 scale, Float2 center = Float2{0, 0}) const noexcept
        {
            const float b_11 = scale.x;
            const float b_22 = scale.y;
            const float b_31 = (1.0f - scale.x) * center.x;
            const float b_32 = (1.0f - scale.y) * center.y;

            return {
                (_11 * b_11), (_12 * b_22),
                (_21 * b_11), (_22 * b_22),
                (_31 * b_11 + b_31), (_32 * b_22 + b_32)
            };
        }

        bool operator==(const Mat3x2& rhs) const noexcept;

        bool operator!=(const Mat3x2& rhs) const noexcept;

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
