#include "pch.h"
#include "EngineEntryPoint.h"

#include "Windows.h"
#include "TY/Buffer3D.h"

#include "TY/Logger.h"
#include "TY/Utils.h"
#include "TY/detail/EngineCore.h"

using namespace TY;
using namespace TY::detail;

namespace
{
}

void Main();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    LogInfo.hr().writeln(L"application start");

    EngineCore::Init();

    LogInfo.hr().writeln(L"message loop start");

    // -----------------------------------------------

    try
    {
        Main();
    }
    catch (const std::exception& e)
    {
        LogInfo.hr().writeln(L"exception occurred: " + ToUtf16(e.what()));
    }

    if (EngineCore::IsInFrame()) EngineCore::EndFrame();

    // -----------------------------------------------

    LogInfo.hr().writeln(L"message loop end");

    EngineCore::Shutdown();

    LogInfo.hr().writeln(L"application end");

    return 0;
}
