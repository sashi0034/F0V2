#include "pch.h"
#include "EngineEntryPoint.h"

#include "Windows.h"

#include "TY/Logger.h"
#include "TY/System.h"
#include "TY/Utils.h"
#include "TY/detail/EngineCore.h"

using namespace TY;
using namespace TY::detail;

extern void Main();

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

    EngineCore::Shutdown();

    LogInfo.hr().writeln(L"application end");

    return 0;
}
