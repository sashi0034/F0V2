#include "pch.h"
#include "EngineEntryPoint.h"

#include "Windows.h"
#include "ZG/Buffer3D.h"

#include "ZG/Logger.h"
#include "ZG/Utils.h"
#include "ZG/detail/EngineCore.h"

using namespace ZG;
using namespace ZG::detail;

namespace
{
}

void Main();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    LogInfo.HR().Writeln(L"application start");

    EngineCore.Init();

    LogInfo.HR().Writeln(L"message loop start");

    // -----------------------------------------------

    try
    {
        Main();
    }
    catch (const std::exception& e)
    {
        LogInfo.HR().Writeln(L"exception occurred: " + ToUtf16(e.what()));
    }

    if (EngineCore.IsInFrame()) EngineCore.EndFrame();

    // -----------------------------------------------

    LogInfo.HR().Writeln(L"message loop end");

    EngineCore.Destroy();

    LogInfo.HR().Writeln(L"application end");

    return 0;
}
