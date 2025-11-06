#pragma once
#include "IGpuMemoryUsage.h"

namespace TY
{
    namespace GpuMetrics
    {
        IGpuMemoryUsage& MemoryUsage();

        float LastExecutionMilliseconds();
    }
}
