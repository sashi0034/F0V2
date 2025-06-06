#include "pch.h"
#include "System.h"

#include "AssertObject.h"
#include "EngineTimer.h"
#include "detail/EngineCore.h"

using namespace TY::detail;

namespace
{
    struct SystemState
    {
        std::optional<double> targetFrameRate{};
    } s_system{};

    void sleepForTargetFrameRate()
    {
        // TODO: Timer 修正
        if (const auto targetFrameRate = s_system.targetFrameRate)
        {
            const double targetDeltaTime = 1.0 / *targetFrameRate;
            const double actualDeltaTime = EngineTimer.GetDeltaTime();
            if (actualDeltaTime < targetDeltaTime)
            {
                const double remaining = targetDeltaTime - actualDeltaTime;
                ::Sleep(static_cast<DWORD>(remaining * 1000.0));
            }
        }
    }
}

namespace TY
{
    using namespace detail;

    void System::SetTargetFrameRate(std::optional<double> frameRate)
    {
        s_system.targetFrameRate = frameRate;
    }

    std::optional<double> System::TargetFrameRate()
    {
        return s_system.targetFrameRate;
    }

    bool System::Update()
    {
        sleepForTargetFrameRate();

        if (EngineCore.IsInFrame()) EngineCore.EndFrame();

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) return false;
        }

        EngineCore.BeginFrame();
        return true;
    }

    double System::DeltaTime()
    {
        return EngineTimer.GetDeltaTime();
    }

    uint64_t System::FrameCount()
    {
        return EngineTimer.GetFrameCount();
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
