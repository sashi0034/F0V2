#include "pch.h"
#include "Mouse.h"

#include "Intersects2D.h"
#include "Scene.h"
#include "detail/EngineKeyboardMouse.h"
#include "detail/EngineWindow.h"

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
        return EngineWindow::GetWheelDelta();
    }

    Float2 Mouse::Drag(MouseInput button)
    {
        Float2 result{};

        if (button.pressed())
        {
            const bool previousInScreen = Intersects(Scene::RectF(), Mouse::PreviousPosF());
            if (previousInScreen)
            {
                result = Mouse::PosF() - Mouse::PreviousPosF();

                const bool currentInScreen = Intersects(Scene::RectF(), Mouse::PosF());
                if (not currentInScreen)
                {
                    Float2 newMousePos = Mouse::PosF();
                    newMousePos.x = Math::Mod(newMousePos.x, Scene::Size().x);
                    newMousePos.y = Math::Mod(newMousePos.y, Scene::Size().y);
                    Mouse::SetPosF(newMousePos);
                }
            }
        }

        return result;
    }
}
