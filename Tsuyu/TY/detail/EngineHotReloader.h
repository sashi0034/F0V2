#pragma once
#include "IEngineHotReloadable.h"
#include "TY/Array.h"

namespace TY::detail
{
    class EngineHotReloader_impl
    {
    public:
        void Update() const;

        void Destroy() const;

        void TrackAsset(std::weak_ptr<IEngineHotReloadable> target, Array<std::shared_ptr<ITimestamp>> dependencies) const;
    };

    static inline constexpr auto EngineHotReloader = EngineHotReloader_impl{};
}
