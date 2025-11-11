#include "pch.h"
#include "InputUtils.h"

#include "Gamepad.h"
#include "KeyboardInput.h"

namespace TY
{
    std::optional<Direction4> KeyboardUtils::GetTriggeredArrow()
    {
        std::optional<Direction4> result{};
        if (KeyUp.down())
        {
            result = Direction4::Up;
        }
        else if (KeyLeft.down())
        {
            result = Direction4::Left;
        }
        else if (KeyDown.down())
        {
            result = Direction4::Down;
        }
        else if (KeyRight.down())
        {
            result = Direction4::Right;
        }

        return result;
    }

    std::optional<Direction4> KeyboardUtils::GetTriggeredWASD()
    {
        std::optional<Direction4> result{};
        if (KeyW.down())
        {
            result = Direction4::Up;
        }
        else if (KeyA.down())
        {
            result = Direction4::Left;
        }
        else if (KeyS.down())
        {
            result = Direction4::Down;
        }
        else if (KeyD.down())
        {
            result = Direction4::Right;
        }

        return result;
    }

    std::optional<Direction4> KeyboardUtils::GetTriggeredArrowOrWASD()
    {
        if (auto dir = GetTriggeredArrow(); dir.has_value())
        {
            return dir;
        }

        return GetTriggeredWASD();
    }

    std::optional<Direction4> GamepadUtils::GetTriggeredDpad()
    {
        std::optional<Direction4> result{};
        if (MainGamepad.dpadUp().down)
        {
            result = Direction4::Up;
        }
        else if (MainGamepad.dpadLeft().down)
        {
            result = Direction4::Left;
        }
        else if (MainGamepad.dpadDown().down)
        {
            result = Direction4::Down;
        }
        else if (MainGamepad.dpadRight().down)
        {
            result = Direction4::Right;
        }

        return result;
    }
}
