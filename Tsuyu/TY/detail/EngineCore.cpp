#include "pch.h"
#include "EngineCore.h"

#include "EngineCacheContext.h"
#include "EngineGamepad.h"
#include "EngineHotReloader.h"
#include "EngineImGUI.h"
#include "EngineKeyboardMouse.h"
#include "EnginePresetAsset.h"
#include "EngineRenderContext.h"
#include "EngineStateContext.h"
#include "EngineTimer.h"
#include "EngineWindow.h"
#include "IEngineDrawer.h"
#include "TY/Array.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

using namespace std::string_view_literals;

namespace TY::detail
{
    // predefined components
    void InitGameTimeComponent();
}

namespace
{
    void initPredefinedComponents()
    {
        InitGameTimeComponent();
    }
}

struct EngineCoreImpl
{
    bool m_inFrame{};

    Array<std::weak_ptr<IEngineUpdatable>> m_updatableList{};

    Array<std::shared_ptr<IEngineDrawer>> m_drawersInFrame{};

    Array<ComponentObject> m_components{};

    void Init()
    {
        EngineTimer::Init();

        EngineWindow::Init();

        EngineRenderContext::Init();

        EngineWindow::Show(); // <-- window will be shown

        EnginePresetAsset::Init();

        EngineGamepad::Init();

        EngineImGui::Init();

        initPredefinedComponents();
    }

    void BeginFrame()
    {
        m_inFrame = true;

        EngineRenderContext::NewFrame();

        EngineImGui::NewFrame();

        EngineTimer::Update();

        EngineCacheContext::Update();

        EngineWindow::Update();

        EngineGamepad::Update();

        EngineHotReloader::Update();

        EngineKeyboardMouse::Update();

        for (auto& updatable : m_updatableList)
        {
            if (const auto updatablePtr = updatable.lock())
            {
                updatablePtr->Update();
            }
        }

        for (auto addon = m_components.begin(); addon != m_components.end();)
        {
            if (not addon->addon->update())
            {
                addon = m_components.erase(addon);
            }
            else
            {
                ++addon;
            }
        }
    }

    void EndFrame()
    {
        EngineImGui::Render();

        EngineRenderContext::Render();

        m_drawersInFrame.clear();

        for (auto& addon : m_components)
        {
            addon.addon->postPresent();
        }

        m_inFrame = false;
    }

    void Shutdown()
    {
        m_updatableList.clear();

        m_drawersInFrame.clear();

        m_components.clear();

        EngineStateContext::Shutdown();

        EngineCacheContext::Shutdown();

        EngineWindow::Shutdown();

        EngineRenderContext::Shutdown();

        EngineHotReloader::Shutdown();

        EnginePresetAsset::Shutdown();

        EngineGamepad::Shutdown();

        EngineImGui::Shutdown();
    }
};

namespace
{
    EngineCoreImpl s_core{};
}

namespace TY
{
    void EngineCore::Init()
    {
        s_core.Init();
    }

    bool EngineCore::IsInFrame()
    {
        return s_core.m_inFrame;
    }

    void EngineCore::BeginFrame()
    {
        s_core.BeginFrame();
    }

    void EngineCore::EndFrame()
    {
        s_core.EndFrame();
    }

    void EngineCore::Shutdown()
    {
        s_core.Shutdown();
    }

    void EngineCore::ObserveUpdatable(const std::weak_ptr<IEngineUpdatable>& updatable)
    {
        s_core.m_updatableList.push_back(updatable);
    }

    void EngineCore::ObserveComponent(ComponentObject addon)
    {
        s_core.m_components.push_back(std::move(addon));
    }

    const Array<ComponentObject>& EngineCore::ComponentList()
    {
        return s_core.m_components;
    }

    void EngineCore::MarkDrawerInFrame(const std::shared_ptr<IEngineDrawer>& updatable)
    {
        if (s_core.m_inFrame)
        {
            s_core.m_drawersInFrame.push_back(updatable);
        }
        else
        {
            LogError.writeln("EngineCore::MarkDrawerInFrame(): Invalid call detected outside the frame lifecycle.");
        }
    }
}
