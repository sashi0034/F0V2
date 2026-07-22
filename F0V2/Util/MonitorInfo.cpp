#include "pch.h"
#include "MonitorInfo.h"

#include "TY/Utils.h"

namespace
{
    BOOL CALLBACK collectMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM data)
    {
        MONITORINFOEXW info{};
        info.cbSize = sizeof(info);
        if (not GetMonitorInfoW(monitor, &info))
        {
            return TRUE;
        }

        auto& monitors = *reinterpret_cast<std::vector<MonitorInfo>*>(data);
        monitors.push_back({
            .deviceName = ToUtf8(info.szDevice),
            .rect = info.rcMonitor,
            .primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0,
        });
        return TRUE;
    }
}

namespace Util_inline
{
    std::vector<MonitorInfo> EnumerateMonitors()
    {
        std::vector<MonitorInfo> monitors{};
        EnumDisplayMonitors(nullptr, nullptr, collectMonitor, reinterpret_cast<LPARAM>(&monitors));

        std::ranges::sort(monitors, [](const MonitorInfo& a, const MonitorInfo& b)
        {
            return a.deviceName < b.deviceName;
        });

        return monitors;
    }
}
