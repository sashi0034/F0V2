#include "pch.h"
#include "Gamepad.h"

#include <windows.h>
#include <dinput.h>
#include <iostream>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

LPDIRECTINPUT8 g_pDI = nullptr;
LPDIRECTINPUTDEVICE8 g_pGamepad = nullptr;

BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
    if (FAILED(g_pDI->CreateDevice(pdidInstance->guidInstance, &g_pGamepad, nullptr)))
        return DIENUM_CONTINUE; // 次のデバイスへ

    return DIENUM_STOP; // 最初に見つかったデバイスで止める
}

bool InitDirectInput(HINSTANCE hInstance, HWND hWnd)
{
    if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&g_pDI, nullptr)))
        return false;

    if (FAILED(g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, nullptr, DIEDFL_ATTACHEDONLY)))
        return false;

    if (g_pGamepad == nullptr)
        return false;

    if (FAILED(g_pGamepad->SetDataFormat(&c_dfDIJoystick)))
        return false;

    if (FAILED(g_pGamepad->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE)))
        return false;

    return true;
}

void PollInput()
{
    if (!g_pGamepad) return;

    DIJOYSTATE js;
    if (FAILED(g_pGamepad->Poll()))
    {
        g_pGamepad->Acquire();
        return;
    }

    if (SUCCEEDED(g_pGamepad->GetDeviceState(sizeof(DIJOYSTATE), &js)))
    {
        std::cout << "X: " << js.lX << ", Y: " << js.lY << std::endl;
        for (int i = 0; i < 32; ++i)
        {
            if (js.rgbButtons[i] & 0x80)
                std::cout << "Button " << i << " pressed." << std::endl;
        }
    }
}

void Cleanup()
{
    if (g_pGamepad) g_pGamepad->Unacquire();
    if (g_pGamepad) g_pGamepad->Release();
    if (g_pDI) g_pDI->Release();
}
