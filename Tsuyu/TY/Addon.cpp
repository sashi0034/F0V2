#include "pch.h"
#include "Addon.h"

#include "Logger.h"
#include "detail/ComponentManager_singleton.h"
#include "detail/EngineCore.h"

using namespace TY::detail;

namespace TY
{
    void Addon::detail::HandleAlreadyRegistered(std::string_view name)
    {
        ComponentManager_singleton::HandleAlreadyRegistered(name);
    }

    bool Addon::detail::InitializeAndRegister(std::string_view name, std::unique_ptr<IAddon> addon)
    {
        return ComponentManager_singleton::RegisterInternal(name, std::move(addon));
    }

    bool Addon::IsRegistered(std::string_view name)
    {
        return ComponentManager_singleton::IsRegistered(name);
    }
}
