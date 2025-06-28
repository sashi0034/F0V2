#pragma once
#include "IAddon.h"

namespace TY
{
    namespace Addon
    {
        namespace detail
        {
            void OnAlreadyRegistered(std::string_view name);

            bool InitializeAndRegister(std::string_view name, std::unique_ptr<IAddon> addon);
        }

        bool IsRegistered(std::string_view name);

        template <class AddonType> requires std::derived_from<AddonType, IAddon>
        bool Register(std::string_view name)
        {
            if (IsRegistered(name))
            {
                detail::OnAlreadyRegistered(name);
                return false;
            }

            return detail::InitializeAndRegister(name, std::make_unique<AddonType>());
        }
    }
}
