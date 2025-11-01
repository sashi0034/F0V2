#pragma once
#include "Rect.h"
#include "Vector2D.h"

namespace TY
{
    namespace Scene
    {
        void RequestResize(Size size);

        [[nodiscard]]
        TY::Size Size();

        [[nodiscard]]
        TY::SizeF SizeF();

        [[nodiscard]]
        Point Center();

        [[nodiscard]]
        TY::Rect Rect();

        [[nodiscard]]
        TY::RectF RectF();

        // -----------------------------------------------

        [[nodiscard]]
        Point TopLeft();

        [[nodiscard]]
        Point TopCenter();

        [[nodiscard]]
        Point TopRight();

        [[nodiscard]]
        Point MiddleLeft();

        [[nodiscard]]
        Point MiddleCenter();

        [[nodiscard]]
        Point MiddleRight();

        [[nodiscard]]
        Point BottomLeft();

        [[nodiscard]]
        Point BottomCenter();

        [[nodiscard]]
        Point BottomRight();

        // -----------------------------------------------

        [[nodiscard]]
        Float2 TopLeftF();

        [[nodiscard]]
        Float2 TopCenterF();

        [[nodiscard]]
        Float2 TopRightF();

        [[nodiscard]]
        Float2 MiddleLeftF();

        [[nodiscard]]
        Float2 MiddleCenterF();

        [[nodiscard]]
        Float2 MiddleRightF();

        [[nodiscard]]
        Float2 BottomLeftF();

        [[nodiscard]]
        Float2 BottomCenterF();

        [[nodiscard]]
        Float2 BottomRightF();
    }
}
