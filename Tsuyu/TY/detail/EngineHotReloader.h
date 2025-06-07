#pragma once
#include "IEngineHotReloadable.h"
#include "TY/Array.h"

namespace TY::detail
{
    namespace EngineHotReloader
    {
        void Update();

        void TrackAsset(std::weak_ptr<IEngineHotReloadable> target, Array<std::shared_ptr<ITimestamp>> dependencies);

        void Shutdown();
    };
}
