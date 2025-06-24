#include "pch.h"
#include "EngineKeyboardMouse.h"

#include "EngineRenderContext.h"
#include "EngineWindow.h"
#include "TY/Vector2D.h"

using namespace TY;
using namespace TY::detail;

struct EngineKeyboardMouseImpl
{
    std::array<BYTE, 256> m_previousState{};
    std::array<BYTE, 256> m_currentState{};

    Float2 m_mousePosInWindow{};
    Float2 m_mousePosInFrameBuffer{};

    Float2 m_previousMousePosInFrameBuffer{};

    void Update()
    {
        // 前回の状態を保存
        std::ranges::copy(m_currentState, std::begin(m_previousState));

        // 現在の状態を取得
        GetKeyboardState(m_currentState.data());

        // マウスの座標を取得
        POINT mousePos{};
        if (GetCursorPos(&mousePos))
        {
            ScreenToClient(EngineWindow::Handle(), &mousePos);

            m_mousePosInWindow.x = static_cast<float>(mousePos.x);
            m_mousePosInWindow.y = static_cast<float>(mousePos.y);

            m_previousMousePosInFrameBuffer = m_mousePosInFrameBuffer;
            m_mousePosInFrameBuffer = EngineRenderContext::WindowToFrameBuffer().transformPoint(m_mousePosInWindow);
        }
    }
};

namespace
{
    EngineKeyboardMouseImpl s_keyboardMouse{};
}

namespace TY::detail
{
    void EngineKeyboardMouse::Update()
    {
        s_keyboardMouse.Update();
    }

    bool EngineKeyboardMouse::KeyDown(uint8_t code)
    {
        return (s_keyboardMouse.m_currentState[code] & 0x80) && not(s_keyboardMouse.m_previousState[code] & 0x80);
    }

    bool EngineKeyboardMouse::KeyPressed(uint8_t code)
    {
        return s_keyboardMouse.m_currentState[code] & 0x80;
    }

    bool EngineKeyboardMouse::KeyUp(uint8_t code)
    {
        return not(s_keyboardMouse.m_currentState[code] & 0x80) && (s_keyboardMouse.m_previousState[code] & 0x80);
    }

    Float2 EngineKeyboardMouse::MousePos()
    {
        return s_keyboardMouse.m_mousePosInFrameBuffer;
    }

    Float2 EngineKeyboardMouse::PreviousMousePos()
    {
        return s_keyboardMouse.m_previousMousePosInFrameBuffer;
    }
}
