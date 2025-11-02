#include "pch.h"
#include "EngineCore.h"

#include "ComponentManager_singleton.h"
#include "EngineGamepad.h"
#include "EngineHotReloader.h"
#include "EngineImGUI.h"
#include "EngineKeyboardMouse.h"
#include "EnginePresetAsset.h"
#include "RenderContext_singleton.h"
#include "EngineStateContext.h"
#include "EngineTimer.h"
#include "Window_singleton.h"
#include "TY/Array.h"

using namespace TY;
using namespace TY::detail;

using namespace std::string_view_literals;

namespace TY::detail
{
    // predefined components
    extern void InitRenderEventComponent();

    extern void InitGameTimeComponent();

    extern void InitGameStepComponent();

    extern void InitializeGraphicsPipelineStateCacheComponent();

    // extern void InitGpgpuCacheComponent();

    extern void InitImmediateDrawerComponent();

    extern void InitFreeTypeContextComponent();
}

namespace
{
    void initPredefinedComponents()
    {
        InitRenderEventComponent();

        InitGameTimeComponent();

        InitGameStepComponent();

        InitializeGraphicsPipelineStateCacheComponent();

        // InitGpgpuCacheComponent();

        InitImmediateDrawerComponent();

        InitFreeTypeContextComponent();
    }
}

struct EngineCoreImpl
{
    bool m_inFrame{};

    Array<std::weak_ptr<IEngineUpdatable>> m_updatableList{};

    void Init()
    {
        EngineTimer::Init();

        Window_singleton::Init();

        RenderContext_singleton::Init();

        Window_singleton::Show(); // <-- window will be shown

        EnginePresetAsset::Init();

        EngineGamepad::Init();

        EngineImGui::Init();

        initPredefinedComponents();
    }

    void BeginFrame()
    {
        m_inFrame = true;

        RenderContext_singleton::NewFrame();

        EngineImGui::NewFrame();

        EngineTimer::Update();

        Window_singleton::Update();

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

        ComponentManager_singleton::Update();
    }

    void EndFrame()
    {
        ComponentManager_singleton::BeforeFlush();

        EngineImGui::Render();

        RenderContext_singleton::Render();

        Window_singleton::AfterPresent();

        ComponentManager_singleton::AfterPresent();

        m_inFrame = false;
    }

    void Shutdown()
    {
        m_updatableList.clear();

        EngineStateContext::Shutdown();

        Window_singleton::Shutdown();

        EngineHotReloader::Shutdown();

        EnginePresetAsset::Shutdown();

        EngineGamepad::Shutdown();

        EngineImGui::Shutdown();

        ComponentManager_singleton::Shutdown();

        // 他のリソースを全て解放してからレンダリングリソースを解放する
        RenderContext_singleton::Shutdown();

        // FIXME: 順序関係?
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
}
