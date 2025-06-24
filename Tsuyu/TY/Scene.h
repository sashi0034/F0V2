#pragma once
#include "Vector2D.h"

namespace TY
{
    namespace Scene
    {
        void RequestResize(Size size);

        [[nodiscard]] TY::Size Size();

        [[nodiscard]] Point Center();
    }
}
