#include "pch.h"
#include "Addon.h"

#include "Logger.h"
#include "detail/EngineCore.h"

using namespace TY::detail;

namespace TY
{
    void Addon::detail::OnAlreadyRegistered(std::string_view name)
    {
        LogError(std::format("Addon '{}' is already registered.", name));
    }

    bool Addon::detail::InitializeAndRegister(std::string_view name, std::unique_ptr<IAddon> addon)
    {
        if (not addon->init())
        {
            LogError(std::format("Failed to initialize addon '{}'.", name));
            return false;
        }

        EngineCore::ObserveAddon({name, std::move(addon)});
        return true;
    }

    bool Addon::IsRegistered(std::string_view name)
    {
        for (const auto& addon : EngineCore::AddonList())
        {
            if (addon.name == name)
            {
                return true;
            }
        }

        return false;
    }
}
