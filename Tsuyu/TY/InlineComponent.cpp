#include "pch.h"
#include "InlineComponent.h"

#include "detail/EngineStateContext.h"

namespace TY
{
    namespace detail
    {
        InlineComponentId InlineComponentId::Next()
        {
            static size_t s_id = 0;
            const auto id = s_id;
            ++s_id;
            return InlineComponentId{id};
        }

        InlineComponentId::InlineComponentId(size_t value) : m_value(value)
        {
        }

        IInlineComponent& FetchInlineComponent(
            InlineComponentId id,
            const std::function<std::unique_ptr<IInlineComponent>()>& initializer)
        {
            return EngineStateContext::FetchInlineComponent(id, initializer);
        }
    }
}
