#pragma once
#include "Vector2D.h"

namespace TY
{
    template <typename VectorType>
    struct Rectangle
    {
        using value_type = VectorType::value_type;
        using position_type = VectorType;

        union
        {
            position_type pos;

            struct
            {
                value_type x;

                value_type y;
            };
        };

        union
        {
            position_type size;

            struct
            {
                value_type w;

                value_type h;
            };
        };

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

        position_type tr() const
        {
            return Float2{pos.x + size.x, pos.y};
        }

        position_type br() const
        {
            return pos + size;
        }

        position_type bl() const
        {
            return Float2{pos.x, pos.y + size.y};
        }

        position_type center() const
        {
            return pos + size / 2;
        }

        value_type leftX() const
        {
            return pos.x;
        }

        value_type rightX() const
        {
            return pos.x + size.x;
        }

        value_type topY() const
        {
            return pos.y;
        }

        value_type bottomY() const
        {
            return pos.y + size.y;
        }
    };

    using Rect = Rectangle<Point>;

    using RectF = Rectangle<Float2>;
}
