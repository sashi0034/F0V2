#include "pch.h"
#include "MonitorConfiguration.h"

#include "TY/Utils.h"

namespace
{
    struct MonitorInfo
    {
        std::string deviceName;
        RECT rect{};
        bool primary{};
    };

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
    std::string SerializeCurrentMonitorConfiguration()
    {
        std::vector<MonitorInfo> monitors{};
        EnumDisplayMonitors(nullptr, nullptr, collectMonitor, reinterpret_cast<LPARAM>(&monitors));

        std::ranges::sort(monitors, [](const MonitorInfo& a, const MonitorInfo& b)
        {
            return a.deviceName < b.deviceName;
        });

        std::string result{};
        for (const auto& monitor : monitors)
        {
            result += std::format("{}:{},{},{},{},{};",
                                  monitor.deviceName,
                                  monitor.rect.left,
                                  monitor.rect.top,
                                  monitor.rect.right,
                                  monitor.rect.bottom,
                                  monitor.primary);
        }
        return result;
    }
}
