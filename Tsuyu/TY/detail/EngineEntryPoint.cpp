#include "pch.h"
#include "EngineEntryPoint.h"

#include "EngineRenderContext.h"
#include "Windows.h"
#include "TY/Buffer3D.h"

#include "TY/Logger.h"
#include "TY/System.h"
#include "TY/Utils.h"
#include "TY/detail/EngineCore.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    void reportLiveObjects(ID3D12Device* device)
    {
        ComPtr<ID3D12DebugDevice> debugDevice;
        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&debugDevice))))
        {
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL);
        }
    }
}

void Main();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    LogInfo.hr().writeln(L"application start");

    EngineCore::Init();

    LogInfo.hr().writeln(L"message loop start");

    // -----------------------------------------------

#ifdef _DEBUG
    Main();
#else
    try
    {
        Main();
    }
    catch (const std::exception& e)
    {
        const auto message = L"An error occurred: " + ToUtf16(e.what());
        LogError.hr().writeln(message);
        System::ModalError(message);
    }
#endif

    if (EngineCore::IsInFrame()) EngineCore::EndFrame();

    // -----------------------------------------------

    LogInfo.hr().writeln(L"message loop end");

    ComPtr<ID3D12Device> device = EngineRenderContext::GetDevice();

    EngineCore::Shutdown();

    reportLiveObjects(device.Get());

    LogInfo.hr().writeln(L"application end");

    return 0;
}
