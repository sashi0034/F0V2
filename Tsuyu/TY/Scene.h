#pragma once
#include "Rect.h"
#include "Vector2D.h"

namespace TY
{
    namespace Scene
    {
        void RequestResize(Size size);

        [[nodiscard]] TY::Size Size();

        [[nodiscard]] Point Center();

        [[nodiscard]] TY::Rect Rect();
    }
}
