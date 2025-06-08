#pragma once
#include "TY/Value2D.h"

namespace TY::detail
{
    namespace EngineWindow
    {
        void Init();

        void Show();

        void Update();

        [[nodiscard]]
        HWND Handle();

        [[nodiscard]]
        Size WindowSize();

        void Shutdown();
    };
}
