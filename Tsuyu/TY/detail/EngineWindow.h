#pragma once
#include "TY/Vector2D.h"

namespace TY::detail
{
    namespace EngineWindow
    {
        void Init();

        bool IsInitialized();

        void Show();

        void Update();

        void AfterPresent();

        void Shutdown();

        [[nodiscard]]
        HINSTANCE HInstance();

        [[nodiscard]]
        HWND Handle();

        [[nodiscard]]
        Size GetSize();

        float GetWheelDelta();

        void Resize(Size size);

        void SetTitle(const std::wstring& title);
    }
}
