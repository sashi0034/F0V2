#pragma once

#include "IEngineDrawer.h"
#include "IEngineUpdatable.h"
#include "TY/Array.h"
#include "TY/IAddon.h"

namespace TY::detail
{
    struct ComponentObject
    {
        std::string_view name;
        std::unique_ptr<IComponent> addon;
    };

    namespace EngineCore
    {
        void Init();

        bool IsInFrame();

        void BeginFrame();

        void EndFrame();

        void Shutdown();

        void ObserveUpdatable(const std::weak_ptr<IEngineUpdatable>& updatable);

        void ObserveComponent(ComponentObject addon);

        const Array<ComponentObject>& ComponentList();

        void MarkDrawerInFrame(const std::shared_ptr<IEngineDrawer>& updatable);
    };
}
