#include "pch.h"
#include "RaceEffectDrawer.h"

#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "TY/Array.h"
#include "TY/GameTime.h"
#include "TY/Logger.h"

using namespace Race;

namespace
{
    struct EffectSystemElement
    {
        std::shared_ptr<IRaceEffectSystem> system{};
        mutable bool drawParametersInitialized{};
    };
}

struct RaceEffectDrawer::Impl : ActorBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
    Array<EffectSystemElement> m_systems{};

    void Init()
    {
        GetRaceContext().registerDrawer(shared_from_this());
    }

    void RegisterEffectSystem(const std::shared_ptr<IRaceEffectSystem>& system)
    {
        assert(system);

        if (not system || findSystem(system.get()) != m_systems.end())
        {
            LogError("RaceEffectDrawer::registerEffectSystem(): Invalid or duplicate system.");
            return;
        }

        system->onRegistered();
        m_systems.push_back(EffectSystemElement{.system = system});
    }

    void UnregisterEffectSystem(const IRaceEffectSystem* system)
    {
        assert(system);

        const auto it = findSystem(system);
        if (it == m_systems.end())
        {
            LogError("RaceEffectDrawer::unregisterEffectSystem(): System not found.");
            return;
        }

        it->system->onUnregistered();
        m_systems.erase(it);
    }

private:
    auto findSystem(const IRaceEffectSystem* system)
    {
        return std::ranges::find_if(
            m_systems,
            [system](const EffectSystemElement& element)
            {
                return element.system.get() == system;
            });
    }

    void update() override
    {
        const auto& camera = GetRaceContextContent().camera;
        const RaceEffectFrameContext context{
            .deltaTime = InGameDeltaTime(),
            .elapsedTime = InGameElapsedTime(),
            .cameraUp = camera.worldMatrix().up(),
            .cameraRight = camera.worldMatrix().right(),
        };

        for (const auto& element : m_systems)
        {
            element.system->update(context);
        }
    }

    void prepareDrawParameters(RaceDrawParameters& parameters, bool init) const override
    {
        for (const auto& element : m_systems)
        {
            element.system->prepareDrawParameters(parameters, not element.drawParametersInitialized);
            element.drawParametersInitialized = true;
        }
    }

    void drawShadowMap() const override
    {
        for (const auto& element : m_systems)
        {
            element.system->drawShadowMap();
        }
    }

    void drawGBuffer() const override
    {
        for (const auto& element : m_systems)
        {
            element.system->drawGBuffer();
        }
    }

    void drawTransparent() const override
    {
        for (const auto& element : m_systems)
        {
            element.system->drawTransparent();
        }
    }

    void drawUI() const override
    {
        for (const auto& element : m_systems)
        {
            element.system->drawUI();
        }
    }

    float orderPriority() const override
    {
        return -500.0f;
    }

    void killed() override
    {
        for (auto it = m_systems.rbegin(); it != m_systems.rend(); ++it)
        {
            it->system->onUnregistered();
        }
        m_systems.clear();

        GetRaceContext().unregisterDrawer(this);
    }
};

namespace Race
{
    RaceEffectDrawer::RaceEffectDrawer() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceEffectDrawer::init()
    {
        p_impl->Init();
    }

    void RaceEffectDrawer::registerEffectSystem(const std::shared_ptr<IRaceEffectSystem>& system)
    {
        p_impl->RegisterEffectSystem(system);
    }

    void RaceEffectDrawer::unregisterEffectSystem(const IRaceEffectSystem* system)
    {
        p_impl->UnregisterEffectSystem(system);
    }

    std::shared_ptr<ActorBase> RaceEffectDrawer::asActor() const
    {
        return p_impl;
    }
}
