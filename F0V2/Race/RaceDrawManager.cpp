#include "pch.h"
#include "RaceDrawManager.h"

#include "Asset.generated.h"
#include "IRaceDrawer.h"
#include "TY/ActorContainer.h"
#include "TY/ComputeDispatcher.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Logger.h"
#include "TY/ModelDrawer.h"
#include "TY/RenderTargetTexture.h"
#include "TY/Screen.h"
#include "TY/System.h"
#include "Util/DebugTomlValue.h"

using namespace Race;

namespace
{
    struct DrawerElement
    {
        std::shared_ptr<IRaceDrawer> drawer;
        bool initialized{};
        RaceDrawParameters parameters{};
    };

    struct Scenery_b10
    {
        Float2 g_outputResolution{};
        Float2 g_mousePosition{};
        Float2 g_mouseUV{};
        float g_time{};
    };

    class SceneryDrawer
    {
    public:
        void Init()
        {
            m_nativeResolution =
                RenderTargetTextureParams()
                .setSize(Screen::Size())
                .setClearColor(ColorF32{0.0f, 1.0f});

            m_nativeResolutionDispatcher =
                ComputeDispatcherParams{}
                .setCS(Asset_shader::scenery1_cs)
                .setCbv({m_cb})
                .setUav({m_nativeResolution});
        }

        void Draw()
        {
            m_cb->g_outputResolution = Screen::Size();
            m_cb->g_time = System::Time();
            m_cb.upload();

            auto& texture = m_nativeResolution;
            const Size textureSize = texture.size();
            const Size threadGroup = (textureSize + Size{7, 7}) / 8;
            m_nativeResolutionDispatcher.dispatch(threadGroup.x, threadGroup.y);

            Immediate2D::Texture(texture).resized(Screen::Size()).pushAuto();
            ImmediateDrawer::Global().draw();
        }

    private:
        ConstantBufferWrapper<Scenery_b10> m_cb{};
        UnorderedRenderTargetTexture m_nativeResolution{};
        ComputeDispatcher m_nativeResolutionDispatcher{};
    };
}

struct RaceDrawManager::Impl : ActorBase
{
#if defined(_DEBUG)
    std::string m_debugName{"RaceDrawManager"};
#endif

    ActorContainer m_children{};

    Array<DrawerElement> m_drawers{};

    SceneryDrawer m_sceneryDrawer{};

    void Init()
    {
        m_sceneryDrawer.Init();
    }

    void Unregister(const IRaceDrawer* drawer)
    {
        for (int i = 0; i < m_drawers.size(); ++i)
        {
            if (m_drawers[i].drawer.get() == drawer)
            {
                m_drawers.erase(m_drawers.begin() + i);
                return;
            }
        }

        LogError("RaceDrawManager::Unregister(): Drawer {} not found.", static_cast<const void*>(drawer));
    }

private:
    void update() override
    {
        for (int i = 0; i < m_drawers.size(); ++i)
        {
            m_drawers[i].drawer->prepareDrawParameters(m_drawers[i].parameters, not m_drawers[i].initialized);
            m_drawers[i].initialized = true;
        }

        for (int i = 0; i < m_drawers.size(); ++i)
        {
            if (m_drawers[i].parameters.drawForward)
            {
                m_drawers[i].drawer->drawForward();
            }
        }

#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("draw_scenery"))
#endif
        {
            m_sceneryDrawer.Draw();
        }

        for (int i = 0; i < m_drawers.size(); ++i)
        {
            if (m_drawers[i].parameters.draw2D)
            {
                m_drawers[i].drawer->draw2D();
            }
        }
    }

    float orderPriority() const override
    {
        return -1000.0f;
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Race
{
    RaceDrawManager::RaceDrawManager() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceDrawManager::init()
    {
        p_impl->Init();
    }

    void RaceDrawManager::registerDrawer(const std::shared_ptr<IRaceDrawer>& drawer)
    {
        assert(drawer != nullptr);
        p_impl->m_drawers.push_back(DrawerElement{drawer, {}});
    }

    void RaceDrawManager::unregisterDrawer(const IRaceDrawer* drawer)
    {
        assert(drawer != nullptr);
        p_impl->Unregister(drawer);
    }

    std::shared_ptr<ActorBase> RaceDrawManager::asActor() const
    {
        return p_impl;
    }
}
