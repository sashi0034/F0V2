#include "pch.h"
#include "EngineKeyboardMouse.h"

#include "RenderContext_singleton.h"
#include "EngineWindow.h"
#include "TY/Vector2D.h"

using namespace TY;
using namespace TY::detail;

struct EngineKeyboardMouseImpl
{
    std::array<BYTE, 256> m_previousState{};
    std::array<BYTE, 256> m_currentState{};

    Array<uint8_t> m_changedCodes{};

    Float2 m_mousePosInWindow{};
    Float2 m_mousePosInFrameBuffer{};

    Float2 m_previousMousePosInFrameBuffer{};

    EngineKeyboardMouseImpl()
    {
        m_changedCodes.reserve(256);
    }

    void Update()
    {
        // 前回の状態を保存
        std::ranges::copy(m_currentState, std::begin(m_previousState));

        // 現在の状態を取得
        GetKeyboardState(m_currentState.data());

        // 状態が変化したキーを収集
        m_changedCodes.clear();
        for (int i = 0; i < m_currentState.size(); ++i)
        {
            if ((m_currentState[i] & 0x80) != (m_previousState[i] & 0x80))
            {
                m_changedCodes.push_back(static_cast<uint8_t>(i));
            }
        }

        // マウスの座標を取得
        POINT mousePos{};
        if (GetCursorPos(&mousePos))
        {
            ScreenToClient(EngineWindow::Handle(), &mousePos);

            m_mousePosInWindow.x = static_cast<float>(mousePos.x);
            m_mousePosInWindow.y = static_cast<float>(mousePos.y);

            m_previousMousePosInFrameBuffer = m_mousePosInFrameBuffer;
            m_mousePosInFrameBuffer = RenderContext_singleton::WindowToFrameBuffer().transformPoint(m_mousePosInWindow);
        }
    }

    void SetMousePosInWindow(const Float2& pos)
    {
        POINT p;
        p.x = static_cast<LONG>(pos.x);
        p.y = static_cast<LONG>(pos.y);
        ClientToScreen(EngineWindow::Handle(), &p);
        SetCursorPos(p.x, p.y);
    }

    void SetMousePosInFrameBuffer(const Float2& pos)
    {
        SetMousePosInWindow(RenderContext_singleton::FrameBufferToWindow().transformPoint(pos));
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

    const Array<uint8_t>& EngineKeyboardMouse::ChangedCodes()
    {
        return s_keyboardMouse.m_changedCodes;
    }

    Float2 EngineKeyboardMouse::MousePos()
    {
        return s_keyboardMouse.m_mousePosInFrameBuffer;
    }

    Float2 EngineKeyboardMouse::PreviousMousePos()
    {
        return s_keyboardMouse.m_previousMousePosInFrameBuffer;
    }

    void EngineKeyboardMouse::SetMousePos(const Float2& pos)
    {
        s_keyboardMouse.SetMousePosInFrameBuffer(pos);
    }
}
