#pragma once

inline namespace Util_inline
{
    void InitDebugTomlValueAddon();

    toml::node_view<toml::node> GetDebugTomlValueInternal();

    template <typename T>
    T GetDebugTomlValue(std::string_view key)
    {
        return GetDebugTomlValueInternal()[key].value_or(T{});
    }
}
