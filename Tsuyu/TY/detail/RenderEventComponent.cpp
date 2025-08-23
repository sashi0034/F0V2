#include "pch.h"
#include "RenderEventComponent.h"

#include "EngineComponent.h"
#include "TY/Array.h"
#include "TY/IComponent.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    struct RenderEventImpl
    {
        Array<std::shared_ptr<RenderEvent::Lister>> m_subscribableList{};
    } s_renderEvent{};

    struct RenderEventComponent : IComponent
    {
        ~RenderEventComponent()
        {
            s_renderEvent = {};
        }

        void beforeFlush() override
        {
            auto& subscribableList = s_renderEvent.m_subscribableList;
            for (auto it = subscribableList.begin(); it != subscribableList.end();)
            {
                it->get()->beforeFlush();

                if (it->get()->shouldRemove())
                {
                    it = subscribableList.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void afterPresent() override
        {
            auto& subscribableList = s_renderEvent.m_subscribableList;
            for (auto it = subscribableList.begin(); it != subscribableList.end();)
            {
                it->get()->afterPresent();
                ++it;
            }
        }
    };
}

namespace TY::detail
{
    void RenderEvent::AddLister(const std::shared_ptr<Lister>& subscribable)
    {
        if (subscribable)
        {
            s_renderEvent.m_subscribableList.push_back(subscribable);
        }
    }

    void InitRenderEventComponent()
    {
        EngineComponent::Register<RenderEventComponent>("RenderEventComponent");
    }
}
