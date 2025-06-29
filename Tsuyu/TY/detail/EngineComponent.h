#pragma once
#include "TY/Array.h"
#include "TY/IComponent.h"

namespace TY::detail
{
    struct ComponentObject
    {
        std::string_view name;
        std::unique_ptr<IComponent> addon;
    };

    namespace EngineComponent
    {
        void Update();

        void PostPresent();

        void Shutdown();

        bool IsRegistered(std::string_view name);

        void HandleAlreadyRegistered(std::string_view name);

        bool RegisterInternal(std::string_view name, std::unique_ptr<IComponent> component);

        const Array<ComponentObject>& ComponentList();

        template <class AddonType> requires std::derived_from<AddonType, IComponent>
        bool Register(std::string_view name)
        {
            if (IsRegistered(name))
            {
                HandleAlreadyRegistered(name);
                return false;
            }

            return RegisterInternal(name, std::make_unique<AddonType>());
        }
    }
}
