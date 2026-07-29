#pragma once

#include "Integer2D.h"
#include "UnifiedString.h"

namespace TY
{
    namespace Window
    {
        void SetTitle(const UnifiedString& title);

        [[nodiscard]]
        Size GetSize();

        void Resize(Size size);

        [[nodiscard]]
        Point GetPosition();

        void SetPosition(Point position);

        void SetForeground();

        [[nodiscard]]
        bool IsActive();
    }
}
