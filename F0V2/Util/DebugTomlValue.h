#pragma once

inline namespace Util_inline
{
#if defined(_DEBUG)
    void InitDebugTomlValueAddon();

    toml::node_view<toml::node> GetDebugTomlValueInternal();

    template <typename T>
    T GetDebugTomlValue(std::string_view key, T defaultValue = T{})
    {
        return GetDebugTomlValueInternal()[key].value_or(defaultValue);
    }
#endif
}
