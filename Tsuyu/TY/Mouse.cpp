#include "pch.h"
#include "Mouse.h"

#include "Intersects2D.h"
#include "Screen.h"
#include "detail/EngineKeyboardMouse.h"
#include "detail/Window_singleton.h"

using namespace TY;
using namespace TY::detail;

namespace TY
{
    Point Mouse::PreviousPos()
    {
        return EngineKeyboardMouse::PreviousMousePos().asPoint();
    }

    Float2 Mouse::PreviousPosF()
    {
        return EngineKeyboardMouse::PreviousMousePos();
    }

    Point Mouse::Pos()
    {
        return EngineKeyboardMouse::MousePos().asPoint();
    }

    Float2 Mouse::PosF()
    {
        return EngineKeyboardMouse::MousePos();
    }

    void Mouse::SetPosF(const Float2& pos)
    {
        EngineKeyboardMouse::SetMousePos(pos);
    }

    float Mouse::Wheel()
    {
        return Window_singleton::WheelDelta();
    }

    Float2 Mouse::Drag(MouseInput button)
    {
        Float2 result{};

        if (button.pressed())
        {
            const bool previousInScreen = Intersects(Screen::RectF(), Mouse::PreviousPosF());
            if (previousInScreen)
            {
                result = Mouse::PosF() - Mouse::PreviousPosF();

                const bool currentInScreen = Intersects(Screen::RectF(), Mouse::PosF());
                if (not currentInScreen)
                {
                    Float2 currentMousePos = Mouse::PosF();
                    Float2 newMousePos = currentMousePos;

                    newMousePos.x = Math::Mod(newMousePos.x, Screen::Size().x);
                    newMousePos.y = Math::Mod(newMousePos.y, Screen::Size().y);

                    // 境界ギリギリの場合をクランプする
                    if (Abs(newMousePos.x) < 1.0f || Abs(newMousePos.x - Screen::Size().x) < 1.0f)
                    {
                        newMousePos.x = currentMousePos.x <= 0 ? Screen::Size().x - 1.0f : 1.0f;
                    }

                    if (Abs(newMousePos.y) < 1.0f || Abs(newMousePos.y - Screen::Size().y) < 1.0f)
                    {
                        newMousePos.y = currentMousePos.y <= 0 ? Screen::Size().y - 1.0f : 1.0f;
                    }

                    Mouse::SetPosF(newMousePos);
                }
            }
        }

        return result;
    }
}
