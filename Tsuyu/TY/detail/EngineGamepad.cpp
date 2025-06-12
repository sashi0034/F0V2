#include "pch.h"
#include "EngineGamepad.h"

#include <windows.h>
#include <dinput.h>

#include "EngineWindow.h"
#include "TY/GamepadInputState.h"

#include "TY/Logger.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace TY;
using namespace TY::detail;

namespace
{
    BOOL CALLBACK DeviceCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext);

    float normalizeAxis(float axis)
    {
        return axis < 0.0 ? axis / 32768.0f : axis / 32767.0f;
    }

    void updateButtonState(GamepadButtonState& button, bool pressed)
    {
        const auto previousPressed = button.pressed;
        button.up = not pressed && previousPressed;
        button.pressed = pressed;
        button.down = pressed && not previousPressed;
    }

    void readPOV(DWORD pov, bool& up, bool& down, bool& left, bool& right)
    {
        const int deg = static_cast<int>(pov / 100);

        up = false;
        down = false;
        left = false;
        right = false;

        if (pov == 0xFFFFFFFF) // POV is not pressed
        {
            return;
        }

        if ((deg >= 315 || deg < 45)) up = true;
        if (deg >= 45 && deg < 135) right = true;
        if (deg >= 135 && deg < 225) down = true;
        if (deg >= 225 && deg < 315) left = true;
    }
}

struct EngineGamepadImpl
{
    LPDIRECTINPUT8 m_di = nullptr;
    LPDIRECTINPUTDEVICE8 m_gamepad = nullptr;

    GamepadInputState m_inputState{};

    void Init()
    {
        assert(EngineWindow::IsInitialized());

        const auto hInstance = EngineWindow::HInstance();
        if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_di, nullptr)))
        {
            return;
        }

        if (FAILED(m_di->EnumDevices(DI8DEVCLASS_GAMECTRL, DeviceCallback, nullptr, DIEDFL_ATTACHEDONLY)))
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

        // -----------------------------------------------

        // 値域を設定
        static const std::array<DWORD, 8> axes = {
            DIJOFS_X,
            DIJOFS_Y,
            DIJOFS_Z,
            DIJOFS_RX,
            DIJOFS_RY,
            DIJOFS_RZ,
            DIJOFS_SLIDER(0),
            DIJOFS_SLIDER(1)
        };

        for (const DWORD axis : axes)
        {
            DIPROPRANGE dipr{};
            dipr.diph.dwSize = sizeof(DIPROPRANGE);
            dipr.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            dipr.diph.dwObj = axis;
            dipr.diph.dwHow = DIPH_BYOFFSET;
            dipr.lMin = -32768;
            dipr.lMax = 32767;

            m_gamepad->SetProperty(DIPROP_RANGE, &dipr.diph);
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
            m_inputState.axisL = Float2{normalizeAxis(js.lX), normalizeAxis(js.lY)};
            m_inputState.axisR = Float2{normalizeAxis(js.lRx), normalizeAxis(js.lRy)};

            bool pressedUp, pressedDown, pressedLeft, pressedRight;
            readPOV(js.rgdwPOV[0], pressedUp, pressedDown, pressedLeft, pressedRight);

            updateButtonState(m_inputState.povUp, pressedUp);
            updateButtonState(m_inputState.povDown, pressedDown);
            updateButtonState(m_inputState.povLeft, pressedLeft);
            updateButtonState(m_inputState.povRight, pressedRight);

            assert(m_inputState.buttons.size() == 32);
            for (int i = 0; i < m_inputState.buttons.size(); ++i)
            {
                const bool pressed = js.rgbButtons[i] & 0x80;
                updateButtonState(m_inputState.buttons[i], pressed);
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

    BOOL CALLBACK DeviceCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
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

    const GamepadInputState& EngineGamepad::GetInputState()
    {
        return s_gamepad.m_inputState;
    }
}
