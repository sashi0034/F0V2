#pragma once

inline namespace Util_inline
{
#if defined(_DEBUG)
    void InitDebugTomlValueAddon();

    [[nodiscard]]
    toml::node_view<toml::node> GetDebugTomlValueInternal();

    template <typename T>
    [[nodiscard]]
    T GetDebugTomlValue(std::string_view key, T defaultValue = T{})
    {
        return GetDebugTomlValueInternal()[key].value_or(defaultValue);
    }

    [[nodiscard]]
    bool IsDebugTomlHotReloaded();
#endif
}
