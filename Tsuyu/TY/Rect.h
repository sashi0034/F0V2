#pragma once
#include "Value2D.h"

namespace TY
{
    template <typename Type>
    struct Rectangle
    {
        using value_type = Type;
        using position_type = Value2D<value_type>;

        position_type pos;
        position_type size;

        [[nodiscard]]
        Rectangle() = default;

        [[nodiscard]]
        Rectangle(const position_type& pos, const position_type& size)
            : pos(pos), size(size)
        {
        }

        [[nodiscard]]
        Rectangle(const position_type& size)
            : pos(0, 0), size(size)
        {
        }

        [[nodiscard]]
        Rectangle(value_type x, value_type y, const position_type& size)
            : pos(x, y), size(size)
        {
        }

        [[nodiscard]]
        Rectangle(const position_type& pos, value_type width, value_type height)
            : pos(pos), size(width, height)
        {
        }

        [[nodiscard]]
        Rectangle(value_type x, value_type y, value_type width, value_type height)
            : pos(x, y), size(width, height)
        {
        }

        position_type tl() const
        {
            return pos;
        }

        position_type br() const
        {
            return pos + size;
        }

        position_type center() const
        {
            return pos + size / 2;
        }
    };

    using RectF = Rectangle<double>;
}
