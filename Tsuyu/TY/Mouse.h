#pragma once
#include "KeyboardMouseInput.h"
#include "Vector2D.h"

namespace TY
{
    class MouseInput : public KeyboardMouseInput
    {
    public:
        using KeyboardMouseInput::KeyboardMouseInput;
    };

    constexpr MouseInput MouseL{VK_LBUTTON};

    constexpr MouseInput MouseR{VK_RBUTTON};

    constexpr MouseInput MouseM{VK_MBUTTON};

    namespace Mouse
    {
        Point Pos();

        Float2 PosF();
    }
}
