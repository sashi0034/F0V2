#pragma once

namespace TY
{
    class KeyboardMouseInput
    {
    public:
        KeyboardMouseInput() = default;

        constexpr KeyboardMouseInput(uint8_t code) : m_code(code)
        {
        }

        bool down() const;

        bool pressed() const;

        bool up() const;

    private:
        uint8_t m_code{};
    };
}
