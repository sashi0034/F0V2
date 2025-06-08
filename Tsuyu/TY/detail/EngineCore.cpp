#include "pch.h"
#include "EngineCore.h"

#include "EngineHotReloader.h"
#include "EngineImGUI.h"
#include "EngineKeyboard.h"
#include "EnginePresetAsset.h"
#include "EngineRenderContext.h"
#include "EngineTimer.h"
#include "EngineWindow.h"
#include "TY/Array.h"

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

    void Init()
    {
        EngineTimer::Init();

        EngineWindow::Init();

        EngineRenderContext::Init();

        EngineWindow::Show(); // <-- window will be shown

        EnginePresetAsset::Init();

        EngineImGui::Init();
    }

    void BeginFrame()
    {
        m_inFrame = true;

        EngineRenderContext::NewFrame();

        EngineImGui::NewFrame();

        EngineTimer::Update();

        EngineWindow::Update();

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

        m_inFrame = false;
    }

    void Shutdown()
    {
        EngineWindow::Shutdown();

        EngineRenderContext::Shutdown();

        EngineHotReloader::Shutdown();

        EnginePresetAsset::Shutdown();

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

    void EngineCore::AddUpdatable(const std::weak_ptr<IEngineUpdatable>& updatable)
    {
        s_core.m_updatableList.push_back(updatable);
    }
}
