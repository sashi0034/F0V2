#include "pch.h"
#include "EngineComponent.h"

#include "EngineCore.h"
#include "TY/Logger.h"

namespace TY::detail
{
    void EngineComponent::HandleAlreadyRegistered(std::string_view name)
    {
        LogError(std::format("Component '{}' is already registered.", name));
    }

    bool EngineComponent::InitializeAndRegister(std::string_view name, std::unique_ptr<IComponent> component)
    {
        if (not component->init())
        {
            LogError(std::format("Failed to initialize addon '{}'.", name));
            return false;
        }

        EngineCore::ObserveComponent({name, std::move(component)});
        return true;
    }

    bool EngineComponent::IsRegistered(std::string_view name)
    {
        for (const auto& component : EngineCore::ComponentList())
        {
            if (component.name == name)
            {
                return true;
            }
        }

        return false;
    }
}
