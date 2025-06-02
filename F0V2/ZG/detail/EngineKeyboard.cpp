#include "pch.h"
#include "EngineKeyboard.h"

namespace
{
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
    } s_impl;
}

namespace ZG::detail
{
    void EngineKeyboard_impl::update() const
    {
        s_impl.Update();
    }

    bool EngineKeyboard_impl::isDown(uint8_t code) const
    {
        return (s_impl.m_currentState[code] & 0x80) && not(s_impl.m_previousState[code] & 0x80);
    }

    bool EngineKeyboard_impl::isPressed(uint8_t code) const
    {
        return s_impl.m_currentState[code] & 0x80;
    }

    bool EngineKeyboard_impl::isUp(uint8_t code) const
    {
        return not(s_impl.m_currentState[code] & 0x80) && (s_impl.m_previousState[code] & 0x80);
    }
}
