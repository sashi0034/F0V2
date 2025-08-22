#include "pch.h"
#include "EngineComponent.h"

#include "EngineCore.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

struct EngineComponentImpl
{
    Array<ComponentObject> m_components{};

    void Update()
    {
        for (auto addon = m_components.begin(); addon != m_components.end();)
        {
            if (not addon->addon->update())
            {
                addon = m_components.erase(addon);
            }
            else
            {
                ++addon;
            }
        }
    }

    void BeforeFlush()
    {
        for (auto& addon : m_components)
        {
            addon.addon->beforeFlush();
        }
    }

    void AfterPresent()
    {
        for (auto& addon : m_components)
        {
            addon.addon->afterPresent();
        }
    }
};

namespace
{
    EngineComponentImpl s_component{};
}

namespace TY::detail
{
    void EngineComponent::HandleAlreadyRegistered(std::string_view name)
    {
        LogError(std::format("Component '{}' is already registered.", name));
    }

    bool EngineComponent::RegisterInternal(std::string_view name, std::unique_ptr<IComponent> component)
    {
        if (not component->init())
        {
            LogError(std::format("Failed to initialize addon '{}'.", name));
            return false;
        }

        s_component.m_components.push_back(ComponentObject{name, std::move(component)});
        return true;
    }

    const Array<ComponentObject>& EngineComponent::ComponentList()
    {
        return s_component.m_components;
    }

    void EngineComponent::Update()
    {
        s_component.Update();
    }

    void EngineComponent::BeforeFlush()
    {
        s_component.BeforeFlush();
    }

    void EngineComponent::AfterPresent()
    {
        s_component.AfterPresent();
    }

    void EngineComponent::Shutdown()
    {
        s_component = {};
    }

    bool EngineComponent::IsRegistered(std::string_view name)
    {
        for (const auto& component : s_component.m_components)
        {
            if (component.name == name)
            {
                return true;
            }
        }

        return false;
    }
}
