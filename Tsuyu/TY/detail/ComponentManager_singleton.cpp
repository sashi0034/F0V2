#include "pch.h"
#include "ComponentManager_singleton.h"

#include "EngineCore.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

struct ComponentManagerImpl
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
    ComponentManagerImpl s_component{};
}

namespace TY::detail
{
    void ComponentManager_singleton::HandleAlreadyRegistered(std::string_view name)
    {
        LogError(std::format("Component '{}' is already registered.", name));
    }

    bool ComponentManager_singleton::RegisterInternal(std::string_view name, std::unique_ptr<IComponent> component)
    {
        if (not component->init())
        {
            LogError(std::format("Failed to initialize addon '{}'.", name));
            return false;
        }

        s_component.m_components.push_back(ComponentObject{name, std::move(component)});
        return true;
    }

    const Array<ComponentObject>& ComponentManager_singleton::ComponentList()
    {
        return s_component.m_components;
    }

    void ComponentManager_singleton::Update()
    {
        s_component.Update();
    }

    void ComponentManager_singleton::BeforeFlush()
    {
        s_component.BeforeFlush();
    }

    void ComponentManager_singleton::AfterPresent()
    {
        s_component.AfterPresent();
    }

    void ComponentManager_singleton::Shutdown()
    {
        s_component = {};
    }

    bool ComponentManager_singleton::IsRegistered(std::string_view name)
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
