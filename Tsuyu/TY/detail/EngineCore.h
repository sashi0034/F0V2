#pragma once

#include "IEngineUpdatable.h"

namespace TY::detail
{
    namespace EngineCore
    {
        void Init();

        bool IsInFrame();

        void BeginFrame();

        void EndFrame();

        void Shutdown();

        void AddUpdatable(const std::weak_ptr<IEngineUpdatable>& updatable);
    };
}
