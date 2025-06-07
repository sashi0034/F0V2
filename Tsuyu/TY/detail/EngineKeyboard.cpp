#include "pch.h"
#include "EngineKeyboard.h"

struct EngineKeyboardImpl
{
    std::array<BYTE, 256> m_previousState{};
    std::array<BYTE, 256> m_currentState{};

    void Update()
    {
        // 前回の状態を保存
        std::ranges::copy(m_currentState, std::begin(m_previousState));

        // 現在の状態を取得
        GetKeyboardState(m_currentState.data());
    }
};

namespace
{
    EngineKeyboardImpl s_keyboard{};
}

namespace TY::detail
{
    void EngineKeyboard::Update()
    {
        s_keyboard.Update();
    }

    bool EngineKeyboard::IsDown(uint8_t code)
    {
        return (s_keyboard.m_currentState[code] & 0x80) && not(s_keyboard.m_previousState[code] & 0x80);
    }

    bool EngineKeyboard::IsPressed(uint8_t code)
    {
        return s_keyboard.m_currentState[code] & 0x80;
    }

    bool EngineKeyboard::IsUp(uint8_t code)
    {
        return not(s_keyboard.m_currentState[code] & 0x80) && (s_keyboard.m_previousState[code] & 0x80);
    }
}
