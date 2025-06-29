#include "pch.h"
#include "Addon.h"

#include "Logger.h"
#include "detail/EngineComponent.h"
#include "detail/EngineCore.h"

using namespace TY::detail;

namespace TY
{
    void Addon::detail::HandleAlreadyRegistered(std::string_view name)
    {
        EngineComponent::HandleAlreadyRegistered(name);
    }

    bool Addon::detail::InitializeAndRegister(std::string_view name, std::unique_ptr<IAddon> addon)
    {
        return EngineComponent::RegisterInternal(name, std::move(addon));
    }

    bool Addon::IsRegistered(std::string_view name)
    {
        return EngineComponent::IsRegistered(name);
    }
}
