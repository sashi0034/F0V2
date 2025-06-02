#pragma once

namespace ZG::detail
{
    class EngineKeyboard_impl
    {
    public:
        void update() const;

        [[nodiscard]] bool isDown(uint8_t code) const;

        [[nodiscard]] bool isPressed(uint8_t code) const;

        [[nodiscard]] bool isUp(uint8_t code) const;
    };

    static inline constexpr auto EngineKeyboard = EngineKeyboard_impl{};
}
