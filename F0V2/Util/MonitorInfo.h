#pragma once

inline namespace Util_inline
{
    struct MonitorInfo
    {
        std::string deviceName;
        RECT rect{};
        bool primary{};
    };

    std::vector<MonitorInfo> EnumerateMonitors();
}
