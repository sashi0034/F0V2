#pragma once

namespace TY::detail
{
    struct ShaderRegisterStart
    {
        int descriptorTableIndex;
        int cbvStart;
        int srvStart;
        int uavStart;

        ShaderRegisterStart() = default;

        explicit ShaderRegisterStart(int tableIndex, int start)
            : descriptorTableIndex(tableIndex), cbvStart(start), srvStart(start), uavStart(start)
        {
        }

        bool operator==(const ShaderRegisterStart&) const = default;
    };
}
