#include "pch.h"
#include "GamepadConfigModal.h"

#include "Asset0.h"
#include "TY/Gamepad.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/KeyboardInput.h"
#include "TY/Screen.h"
#include "TY/System.h"
#include "TY/Utils.h"
#include "Util/ImmediatePrint.h"
#include "Util/Utilities.h"

namespace
{
    void drawText(const Array<RectF>& rects, int i, const std::u32string& text)
    {
        constexpr float fontSize = 24.0f;
        Immediate2D_Text::ZXProto_Sdf(text)
            .setSize(fontSize)
            .setPosition(rects[i].middleLeft(), Alignment9::MiddleLeft)
            .setColor(ColorF32{1.0f, 1.0f, 1.0f})
            .pushAuto();
    }

    class GamepadConfigModal
    {
        std::string m_savePath;
        GamepadMapping m_mapping{};
        int m_nextIndex{};
        bool m_completed{};

    public:
        GamepadConfigModal(std::string savePath)
            : m_savePath(std::move(savePath)),
              m_mapping(GamepadMapping::FromTomlFile(m_savePath))
        {
        }

        void Update()
        {
            auto rects =
                Util::SliceRectByLength(Screen::RectF().stretched({-400.0f, -200.0f}), 40.0f, Direction2::Vertical);

            Immediate2D::Rect{Screen::SizeF()}.setColor(ColorF32{0.1f}).pushAuto();

            drawText(rects, 0, U"[ Gamepad Config ]");

            auto& gamepad = MainGamepad;
            drawButtonMessage(rects, 1, 0, m_mapping.a, "A");
            drawButtonMessage(rects, 2, 1, m_mapping.b, "B");
            drawButtonMessage(rects, 3, 2, m_mapping.x, "X");
            drawButtonMessage(rects, 4, 3, m_mapping.y, "Y");
            drawButtonMessage(rects, 5, 4, m_mapping.lb, "LB");
            drawButtonMessage(rects, 6, 5, m_mapping.rb, "RB");
            drawButtonMessage(rects, 7, 6, m_mapping.lt, "LT");
            drawButtonMessage(rects, 8, 7, m_mapping.rt, "RT");
            drawButtonMessage(rects, 9, 8, m_mapping.menu, "Menu");
            drawButtonMessage(rects, 10, 9, m_mapping.view, "View");
            drawButtonMessage(rects, 11, 10, m_mapping.axis_lx, "Axis: Left Stick X");
            drawButtonMessage(rects, 12, 11, m_mapping.axis_ly, "Axis: Left Stick Y");
            drawButtonMessage(rects, 13, 12, m_mapping.axis_rx, "Axis: Right Stick X");
            drawButtonMessage(rects, 14, 13, m_mapping.axis_ry, "Axis: Right Stick Y");
            if (m_nextIndex == 14)
            {
                drawText(rects, 15, U"Press [ A ] to finish.");

                if (gamepad.rawState().buttons[m_mapping.a].down)
                {
                    m_completed = true;
                    m_mapping.writeToTomlFile(m_savePath);
                    MainGamepad.registerMapping(m_mapping);
                    return;
                }
            }

            drawText(rects, rects.size() - 1, U"Escape: Exit without saving | Backspace: Go back");

            if (KeyBackspace.down() && m_nextIndex > 0)
            {
                m_nextIndex--;
            }

            if (const auto downButtons = gamepad.rawState().getDownButtonIndexes();
                not downButtons.empty())
            {
                bool through{};
                switch (m_nextIndex)
                {
                case 0:
                    m_mapping.a = downButtons[0];
                    break;
                case 1:
                    m_mapping.b = downButtons[0];
                    break;
                case 2:
                    m_mapping.x = downButtons[0];
                    break;
                case 3:
                    m_mapping.y = downButtons[0];
                    break;
                case 4:
                    m_mapping.lb = downButtons[0];
                    break;
                case 5:
                    m_mapping.rb = downButtons[0];
                    break;
                case 6:
                    m_mapping.lt = downButtons[0];
                    break;
                case 7:
                    m_mapping.rt = downButtons[0];
                    break;
                case 8:
                    m_mapping.menu = downButtons[0];
                    break;
                case 9:
                    m_mapping.view = downButtons[0];
                    break;
                default:
                    through = true;
                    break;
                }

                if (not through)
                {
                    m_nextIndex++;
                }
            }

            if (const auto activeAxis = gamepad.rawState().getActiveAxisIndexes();
                not activeAxis.empty())
            {
                switch (m_nextIndex)
                {
                case 10:
                    m_mapping.axis_lx = activeAxis[0];
                    m_nextIndex++;
                    break;
                case 11:
                    if (m_mapping.axis_lx == activeAxis[0]) break;
                    m_mapping.axis_ly = activeAxis[0];
                    m_nextIndex++;
                    break;
                case 12:
                    if (m_mapping.axis_ly == activeAxis[0]) break;
                    m_mapping.axis_rx = activeAxis[0];
                    m_nextIndex++;
                    break;
                case 13:
                    if (m_mapping.axis_rx == activeAxis[0]) break;
                    m_mapping.axis_ry = activeAxis[0];
                    m_nextIndex++;
                    break;
                default:
                    break;
                }
            }

            ImmediateDrawer::Global().draw();
        }

        bool IsFinished() const
        {
            return KeyEscape.down() || m_completed;
        }

    private:
        void drawButtonMessage(
            const Array<RectF>& rects, int row, int buttonIndex, int buttonValue, const std::string& buttonLabel) const
        {
            std::string message;
            if (buttonIndex < m_nextIndex)
            {
                message = std::format("         [ {} ]: {}", buttonLabel, buttonValue);
            }
            else if (buttonIndex == m_nextIndex)
            {
                message = std::format("Next --> [ {} ]:", buttonLabel);
            }
            else // buttonIndex > m_nextIndex
            {
                message = std::format("         [ {} ]:", buttonLabel);
            }

            drawText(rects, row, ToUtf32(message));
        }
    };
}

void GM::LaunchGamepadConfigModal(const std::string& savePath)
{
    GamepadConfigModal modal{savePath};

    while (System::Update())
    {
        if (modal.IsFinished())
        {
            break;
        }

        modal.Update();
    }
}
