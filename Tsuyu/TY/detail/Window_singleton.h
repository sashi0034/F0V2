#pragma once
#include "TY/Vector2D.h"

namespace TY::detail
{
    namespace Window_singleton
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

        [[nodiscard]]
        int TitleBarHeight();

        [[nodiscard]]
        float WheelDelta();

        [[nodiscard]]
        bool IsActive();

        void Resize(Size size);

        void SetTitle(const std::wstring& title);
    }
}
