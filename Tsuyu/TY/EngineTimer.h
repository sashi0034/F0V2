#pragma once

namespace TY::detail
{
    class EngineTimer_impl
    {
    public:
        void Reset() const;

        void Tick() const;

        double GetDeltaTime() const;

        uint64_t GetFrameCount() const;
    };

    inline constexpr auto EngineTimer = EngineTimer_impl{};
}
