#pragma once

namespace ZG::detail
{
    class EngineImGui_impl
    {
    public:
        void init() const;

        void newFrame() const;

        void render() const;
    };

    static inline constexpr auto EngineImGui = EngineImGui_impl{};
}
