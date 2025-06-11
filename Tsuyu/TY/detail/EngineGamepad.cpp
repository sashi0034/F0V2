#include "pch.h"
#include "EngineGamepad.h"

#include <windows.h>
#include <dinput.h>

#include "EngineWindow.h"
#include "TY/Logger.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace TY;
using namespace TY::detail;

namespace
{
    BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext);
}

struct EngineGamepadImpl
{
    LPDIRECTINPUT8 m_di = nullptr;
    LPDIRECTINPUTDEVICE8 m_gamepad = nullptr;

    void Init()
    {
        assert(EngineWindow::IsInitialized());

        const auto hInstance = EngineWindow::HInstance();
        if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_di, nullptr)))
        {
            return;
        }

        if (FAILED(m_di->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, nullptr, DIEDFL_ATTACHEDONLY)))
        {
            return;
        }

        if (m_gamepad == nullptr)
        {
            return;
        }

        if (FAILED(m_gamepad->SetDataFormat(&c_dfDIJoystick)))
        {
            return;
        }

        const auto hwnd = EngineWindow::Handle();
        if (FAILED(m_gamepad->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE)))
        {
            return;
        }
    }

    void Update()
    {
        if (!m_gamepad) return;

        DIJOYSTATE js;
        if (FAILED(m_gamepad->Poll()))
        {
            m_gamepad->Acquire();
            return;
        }

        if (SUCCEEDED(m_gamepad->GetDeviceState(sizeof(DIJOYSTATE), &js)))
        {
            std::cout << "X: " << js.lX << ", Y: " << js.lY << std::endl;
            for (int i = 0; i < 32; ++i)
            {
                if (js.rgbButtons[i] & 0x80)
                    std::cout << "Button " << i << " pressed." << std::endl;
            }
        }
    }

    void Shutdown()
    {
        if (m_gamepad) m_gamepad->Unacquire();
        if (m_gamepad) m_gamepad->Release();
        if (m_di) m_di->Release();
    }
};

namespace
{
    EngineGamepadImpl s_gamepad{};

    BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
    {
        if (FAILED(s_gamepad.m_di->CreateDevice(pdidInstance->guidInstance, &s_gamepad.m_gamepad, nullptr)))
        {
            // 次のデバイスへ
            return DIENUM_CONTINUE;
        }

        // 最初に見つかったデバイスで止める
        return DIENUM_STOP;
    }
}

namespace TY::detail
{
    void EngineGamepad::Init()
    {
        s_gamepad.Init();
    }

    void EngineGamepad::Update()
    {
        s_gamepad.Update();
    }

    void EngineGamepad::Shutdown()
    {
        s_gamepad.Shutdown();
        s_gamepad = {};
    }
}
