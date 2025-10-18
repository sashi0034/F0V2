#pragma once

#include "TY/InlineComponent.h"
#include "TY/Mat4x4.h"

namespace TY::detail
{
    namespace EngineStateContext
    {
        void Shutdown();

        [[nodiscard]]
        IInlineComponent& FetchInlineComponent(
            InlineComponentId id,
            const std::function<std::unique_ptr<IInlineComponent>()>& initializer);
    };
}
