#pragma once

namespace TY::detail
{
    class EngineImGui_impl
    {
    public:
        void init() const;

        void newFrame() const;

        void render() const;

        void destroy() const;
    };

    static inline constexpr auto EngineImGui = EngineImGui_impl{};
}
