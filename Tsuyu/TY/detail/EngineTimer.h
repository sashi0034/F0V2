#pragma once

namespace TY::detail
{
    namespace EngineTimer
    {
        void Init();

        void Update();

        float GetDeltaTime();

        uint64_t GetFrameCount();
    };
}
