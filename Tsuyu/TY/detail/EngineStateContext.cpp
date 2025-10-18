#include "pch.h"
#include "EngineStateContext.h"

#include "TY/Array.h"

using namespace TY;

struct EngineStateContextImpl
{
    Array<std::unique_ptr<IInlineComponent>> m_components{};
};

namespace
{
    EngineStateContextImpl s_stateContext;
}

namespace TY::detail
{
    void EngineStateContext::Shutdown()
    {
        s_stateContext = {};
    }

    IInlineComponent& EngineStateContext::FetchInlineComponent(
        InlineComponentId id,
        const std::function<std::unique_ptr<IInlineComponent>()>& initializer)
    {
        if (s_stateContext.m_components.size() <= id.value())
        {
            s_stateContext.m_components.resize(id.value() + 1);
        }

        if (s_stateContext.m_components[id.value()] == nullptr)
        {
            s_stateContext.m_components[id.value()] = initializer();
        }

        return *s_stateContext.m_components[id.value()];
    }
}
