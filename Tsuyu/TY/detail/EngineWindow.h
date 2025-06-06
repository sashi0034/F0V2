#pragma once
#include "TY/Value2D.h"

namespace TY::detail
{
    class EngineWindow_impl
    {
    public:
        void Init() const;

        void Show() const;

        void Update() const;

        [[nodiscard]]
        HWND Handle() const;

        [[nodiscard]]
        Size WindowSize() const;

        void Destroy() const;
    };

    constexpr inline auto EngineWindow = EngineWindow_impl{};
}
