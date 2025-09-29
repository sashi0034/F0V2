#pragma once
#include <dxgi1_6.h>

#include "TY/IGpuMemoryUsage.h"

namespace TY::detail
{
    class GpuMemoryUsage : public IGpuMemoryUsage
    {
    public:
        GpuMemoryUsage() = default;

        GpuMemoryUsage(IDXGIFactory6* factory, LUID adapterLuid);

        uint64_t wholeLocalUsage() const override;

        uint64_t wholeNonLocalUsage() const override;

        uint64_t estimateLocalUsage() const override;

        uint64_t estimateNonLocalUsageDelta() const override;

        void resetBaseline();

    private:
        uint64_t queryUsage(DXGI_MEMORY_SEGMENT_GROUP group) const;

        ComPtr<IDXGIFactory6> m_factory;
        LUID m_adapterLuid{};

        uint64_t m_baseLocalUsage{};
        uint64_t m_baseNonLocalUsage{};
    };
}
