#pragma once

namespace TY
{
    class IGpuMemoryUsage
    {
    public:
        virtual ~IGpuMemoryUsage() = default;

        virtual uint64_t wholeLocalUsage() const = 0;

        virtual uint64_t wholeNonLocalUsage() const = 0;

        virtual uint64_t estimateLocalUsage() const = 0;

        virtual uint64_t estimateNonLocalUsageDelta() const = 0;

        float estimateLocalUsageInMB() const
        {
            return static_cast<float>(estimateLocalUsage()) / (1024.0f * 1024.0f);
        }

        float estimateNonLocalUsageDeltaInMB() const
        {
            return static_cast<float>(estimateNonLocalUsageDelta()) / (1024.0f * 1024.0f);
        }
    };
}
