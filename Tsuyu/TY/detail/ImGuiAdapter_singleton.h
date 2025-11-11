#pragma once

namespace TY::detail
{
    namespace ImGuiAdapter_singleton
    {
        void Init();

        void NewFrame();

        void Render();

        void Shutdown();
    };
}
