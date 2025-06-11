#pragma once
#include "TY/Value2D.h"

namespace TY::detail
{
    namespace EngineWindow
    {
        void Init();

        bool IsInitialized();

        void Show();

        void Update();

        void Shutdown();

        [[nodiscard]]
        HINSTANCE HInstance();

        [[nodiscard]]
        HWND Handle();

        [[nodiscard]]
        Size WindowSize();

        void SetTitle(const std::wstring& title);
    };
}
