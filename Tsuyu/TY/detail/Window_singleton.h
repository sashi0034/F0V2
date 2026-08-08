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
        Point GetPosition();

        [[nodiscard]]
        int TitleBarHeight();

        [[nodiscard]]
        float WheelDelta();

        [[nodiscard]]
        bool IsActive();

        void Resize(Size size);

        void SetPosition(Point position);

        void SetTitle(const std::wstring& title);
    }
}
