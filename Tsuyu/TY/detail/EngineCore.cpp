#include "pch.h"
#include "EngineCore.h"

#include "EngineGamepad.h"
#include "EngineHotReloader.h"
#include "EngineImGUI.h"
#include "EngineKeyboard.h"
#include "EnginePresetAsset.h"
#include "EngineRenderContext.h"
#include "EngineTimer.h"
#include "EngineWindow.h"
#include "IEngineDrawer.h"
#include "TY/Array.h"
#include "TY/Logger.h"

namespace
{
    using namespace TY;
    using namespace TY::detail;

    using namespace std::string_view_literals;
}

struct EngineCoreImpl
{
    bool m_inFrame{};

    Array<std::weak_ptr<IEngineUpdatable>> m_updatableList{};

    Array<std::shared_ptr<IEngineDrawer>> m_drawersInFrame{};

    void Init()
    {
        EngineTimer::Init();

        EngineWindow::Init();

        EngineRenderContext::Init();

        EngineWindow::Show(); // <-- window will be shown

        EnginePresetAsset::Init();

        EngineGamepad::Init();

        EngineImGui::Init();
    }

    void BeginFrame()
    {
        m_inFrame = true;

        EngineRenderContext::NewFrame();

        EngineImGui::NewFrame();

        EngineTimer::Update();

        EngineWindow::Update();

        EngineGamepad::Update();

        EngineHotReloader::Update();

        EngineKeyboard::Update();

        for (auto& updatable : m_updatableList)
        {
            if (const auto updatablePtr = updatable.lock())
            {
                updatablePtr->Update();
            }
        }
    }

    void EndFrame()
    {
        EngineImGui::Render();

        EngineRenderContext::Render();

        m_drawersInFrame.clear();

        m_inFrame = false;
    }

    void Shutdown()
    {
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
