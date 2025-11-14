#include "pch.h"
#include "ImGuiAdapter_singleton.h"

#include "DescriptorHeap.h"
#include "EngineCore.h"
#include "RenderContext_singleton.h"
#include "Window_singleton.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"

using namespace TY;
using namespace TY::detail;

struct ImGuiAdapterImpl
{
    DescriptorHeap m_descriptorHeap{};

    ComPtr<ID3D12DescriptorHeap> m_srvHeap{};

    void Init()
    {
        IMGUI_CHECKVERSION();

        ImGui::CreateContext();

        ImGui_ImplWin32_Init(Window_singleton::Handle());

        constexpr int framesInFlight = RenderContext_singleton::FrameBufferCount;
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1 * framesInFlight;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvHeapDesc.NodeMask = 0;

        RenderContext_singleton::GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));

        ImGui_ImplDX12_Init(
            RenderContext_singleton::GetDevice(),
            framesInFlight,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            m_srvHeap.Get(),
            m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_srvHeap->GetGPUDescriptorHandleForHeapStart()
        );

        // -----------------------------------------------

        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("asset/engine/font/0xProto/0xProto-Regular.ttf", 14.0f);

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        io.Fonts->Build();
    }
};

namespace
{
    ImGuiAdapterImpl s_imgui{};
}

namespace TY::detail
{
    void ImGuiAdapter_singleton::Init()
    {
        s_imgui.Init();
    }

    void ImGuiAdapter_singleton::NewFrame()
    {
        const auto windowSize = Window_singleton::GetSize();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = windowSize.cast<ImVec2>();

        io.DisplayFramebufferScale =
            (Float2(RenderContext_singleton::FrameBufferSize()) / Float2(Window_singleton::GetSize())).cast<ImVec2>();

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiAdapter_singleton::Render()
    {
        ImGui::Render();

        const auto commandList = RenderContext_singleton::TargetCommandList();
        commandList->SetDescriptorHeaps(1, s_imgui.m_srvHeap.GetAddressOf());

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    }

    void ImGuiAdapter_singleton::Shutdown()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        s_imgui = {};
    }
}
