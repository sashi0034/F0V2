#include "pch.h"
#include "GpuMemoryUsage.h"

namespace TY::detail
{
    GpuMemoryUsage::GpuMemoryUsage(IDXGIFactory6* factory, LUID adapterLuid)
        : m_factory(factory),
          m_adapterLuid(adapterLuid)
    {
        // 起動時のベースラインを記録
        m_baseLocalUsage = GpuMemoryUsage::wholeLocalUsage();
        m_baseNonLocalUsage = GpuMemoryUsage::wholeNonLocalUsage();
    }

    uint64_t GpuMemoryUsage::wholeLocalUsage() const
    {
        return queryUsage(DXGI_MEMORY_SEGMENT_GROUP_LOCAL);
    }

    uint64_t GpuMemoryUsage::wholeNonLocalUsage() const
    {
        return queryUsage(DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL);
    }

    uint64_t GpuMemoryUsage::estimateLocalUsage() const
    {
        uint64_t current = wholeLocalUsage();
        return (current > m_baseLocalUsage) ? (current - m_baseLocalUsage) : 0;
    }

    uint64_t GpuMemoryUsage::estimateNonLocalUsageDelta() const
    {
        uint64_t current = wholeNonLocalUsage();
        return (current > m_baseNonLocalUsage) ? (current - m_baseNonLocalUsage) : 0;
    }

    void GpuMemoryUsage::resetBaseline()
    {
        m_baseLocalUsage = wholeLocalUsage();
        m_baseNonLocalUsage = wholeNonLocalUsage();
    }

    uint64_t GpuMemoryUsage::queryUsage(DXGI_MEMORY_SEGMENT_GROUP group) const
    {
        if (m_factory == nullptr)
        {
            return 0;
        }

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 desc{};
            if (FAILED(adapter->GetDesc1(&desc)))
            {
                continue;
            }

            if (desc.AdapterLuid.HighPart == m_adapterLuid.HighPart &&
                desc.AdapterLuid.LowPart == m_adapterLuid.LowPart)
            {
                ComPtr<IDXGIAdapter4> adapter4;
                if (SUCCEEDED(adapter.As(&adapter4)))
                {
                    DXGI_QUERY_VIDEO_MEMORY_INFO info{};
                    if (SUCCEEDED(adapter4->QueryVideoMemoryInfo(0, group, &info)))
                    {
                        return info.CurrentUsage;
                    }
                }
            }
        }

        return 0;
    }
}
