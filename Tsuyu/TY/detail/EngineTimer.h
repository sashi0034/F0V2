#pragma once

namespace TY::detail
{
    namespace EngineTimer
    {
        void Init();

        void Update();

        double GetDeltaTime();

        uint64_t GetFrameCount();
    };
}
