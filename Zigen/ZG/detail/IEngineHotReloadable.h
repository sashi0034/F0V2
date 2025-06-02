#pragma once
#include "ZG/ITimestamp.h"

namespace ZG::detail
{
    class IEngineHotReloadable : public ITimestamp
    {
    public:
        virtual void HotReload() = 0;
    };
}
