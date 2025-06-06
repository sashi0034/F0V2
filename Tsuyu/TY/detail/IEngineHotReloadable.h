#pragma once
#include "TY/ITimestamp.h"

namespace TY::detail
{
    class IEngineHotReloadable : public ITimestamp
    {
    public:
        virtual void HotReload() = 0;
    };
}
