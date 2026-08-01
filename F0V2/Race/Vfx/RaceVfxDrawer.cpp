#include "pch.h"
#include "RaceVfxDrawer.h"

#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "TY/Array.h"
#include "TY/GameTime.h"
#include "TY/Logger.h"

using namespace Race;

namespace
{
    struct VfxSystemElement
    {
        std::shared_ptr<IRaceVfxSystem> system{};
        mutable bool drawParametersInitialized{};
    };
}

struct RaceVfxDrawer::Impl : ActorBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
    Array<VfxSystemElement> m_systems{};

    void Init()
    {
        GetRaceContext().registerDrawer(shared_from_this());
    }

    void RegisterVfxSystem(const std::shared_ptr<IRaceVfxSystem>& system)
    {
        assert(system);

        if (not system || findSystem(system.get()) != m_systems.end())
        {
            LogError("RaceVfxDrawer::registerVfxSystem(): Invalid or duplicate system.");
            return;
        }

        system->onRegistered();
        m_systems.push_back(VfxSystemElement{.system = system});
    }

    void UnregisterVfxSystem(const IRaceVfxSystem* system)
    {
        assert(system);

        const auto it = findSystem(system);
        if (it == m_systems.end())
        {
            LogError("RaceVfxDrawer::unregisterVfxSystem(): System not found.");
            return;
        }

        it->system->onUnregistered();
        m_systems.erase(it);
    }

private:
    auto findSystem(const IRaceVfxSystem* system)
    {
        return std::ranges::find_if(
            m_systems,
            [system](const VfxSystemElement& element)
            {
                return element.system.get() == system;
            });
    }

    void update() override
    {
        const auto& camera = GetRaceContextContent().camera;
        const RaceVfxFrameContext context{
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
    RaceVfxDrawer::RaceVfxDrawer() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceVfxDrawer::init()
    {
        p_impl->Init();
    }

    void RaceVfxDrawer::registerVfxSystem(const std::shared_ptr<IRaceVfxSystem>& system)
    {
        p_impl->RegisterVfxSystem(system);
    }

    void RaceVfxDrawer::unregisterVfxSystem(const IRaceVfxSystem* system)
    {
        p_impl->UnregisterVfxSystem(system);
    }

    std::shared_ptr<ActorBase> RaceVfxDrawer::asActor() const
    {
        return p_impl;
    }
}
