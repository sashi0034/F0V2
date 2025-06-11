#include "pch.h"
#include "System.h"

#include "AssertObject.h"
#include "detail/EngineTimer.h"
#include "detail/EngineCore.h"

using namespace TY::detail;

namespace
{
    struct SystemState
    {
    } s_system{};
}

namespace TY
{
    using namespace detail;

    bool System::Update()
    {
        if (EngineCore::IsInFrame()) EngineCore::EndFrame();

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) return false;
        }

        EngineCore::BeginFrame();
        return true;
    }

    float System::DeltaTime()
    {
        return EngineTimer::GetDeltaTime();
    }

    uint64_t System::FrameCount()
    {
        return EngineTimer::GetFrameCount();
    }

    void System::Sleep(uint64_t ms)
    {
        if (ms > 0)
        {
            ::Sleep(static_cast<DWORD>(ms));
        }
    }

    void System::ModalError(const std::wstring& message)
    {
        MessageBox(nullptr, message.c_str(), L"assertion failed", MB_OK | MB_ICONERROR);
    }

    void System::ModalError(const std::string& message)
    {
        MessageBoxA(nullptr, message.c_str(), "assertion failed", MB_OK | MB_ICONERROR);
    }
}
