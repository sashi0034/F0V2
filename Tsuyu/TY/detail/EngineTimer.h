#pragma once

namespace TY::detail
{
    namespace EngineTimer
    {
        void Reset();

        void Tick();

        double GetDeltaTime();

        uint64_t GetFrameCount();
    };
}
