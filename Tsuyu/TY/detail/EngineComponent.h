#pragma once
#include "TY/IComponent.h"

namespace TY::detail
{
    namespace EngineComponent
    {
        bool IsRegistered(std::string_view name);

        void HandleAlreadyRegistered(std::string_view name);

        bool InitializeAndRegister(std::string_view name, std::unique_ptr<IComponent> component);

        template <class AddonType> requires std::derived_from<AddonType, IComponent>
        bool Register(std::string_view name)
        {
            if (IsRegistered(name))
            {
                HandleAlreadyRegistered(name);
                return false;
            }

            return InitializeAndRegister(name, std::make_unique<AddonType>());
        }
    }
}
