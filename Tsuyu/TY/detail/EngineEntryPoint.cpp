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
    LogInfo.HR().Writeln(L"application start");

    EngineCore::Init();

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

    if (EngineCore::IsInFrame()) EngineCore::EndFrame();

    // -----------------------------------------------

    LogInfo.HR().Writeln(L"message loop end");

    EngineCore::Shutdown();

    LogInfo.HR().Writeln(L"application end");

    return 0;
}
