#include "pch.h"
#include "GpuMetrics.h"

#include "detail/RenderContext_singleton.h"

using namespace TY;
using namespace TY::detail;

namespace TY
{
    IGpuMemoryUsage& GpuMetrics::MemoryUsage()
    {
        return RenderContext_singleton::MemoryUsage();
    }

    float GpuMetrics::LastExecutionMilliseconds()
    {
        return RenderContext_singleton::LastExecutionMilliseconds();
    }
}
