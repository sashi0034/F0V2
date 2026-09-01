#pragma once
namespace TY::detail
{
    struct DynamicDescriptorEntry
    {
        static constexpr int AutoSlot = -1;

        int cbvSlot{AutoSlot};
        int cbvCount{};

        int srvSlot{AutoSlot};
        int srvCount{};

        int uavSlot{AutoSlot};
        int uavCount{};

        bool operator==(const DynamicDescriptorEntry& other) const = default;
    };
}
