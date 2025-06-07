#include "pch.h"
#include "EngineImGUI.h"

#include "DescriptorHeap.h"
#include "EngineCore.h"
#include "EngineWindow.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"

using namespace TY;
using namespace TY::detail;

struct EngineImGuiImpl
{
    DescriptorHeap m_descriptorHeap{};

    ComPtr<ID3D12DescriptorHeap> m_srvHeap{};

    void Init()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        // ImGuiIO& io = ImGui::GetIO();
        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(EngineWindow::Handle());

        const int framesInFlight = EngineCore.GetBackBuffer().bufferCount();
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1 * framesInFlight;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvHeapDesc.NodeMask = 0;

        EngineCore.GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));

        ImGui_ImplDX12_Init(
            EngineCore.GetDevice(),
            framesInFlight,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            m_srvHeap.Get(),
            m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_srvHeap->GetGPUDescriptorHandleForHeapStart()
        );
    }
};

namespace
{
    EngineImGuiImpl s_imgui{};
}

namespace TY::detail
{
    void EngineImGui::Init()
    {
        s_imgui.Init();
    }

    void EngineImGui::NewFrame()
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void EngineImGui::Render()
    {
        ImGui::Render();

        EngineCore.GetCommandList()->SetDescriptorHeaps(1, s_imgui.m_srvHeap.GetAddressOf());

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), EngineCore.GetCommandList());
    }

    void EngineImGui::Shutdown()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        s_imgui = {};
    }
}
