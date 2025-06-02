#pragma once
#include "ZG/Value2D.h"

namespace ZG::detail
{
    class EngineWindow_impl
    {
    public:
        void Init() const;

        void Show() const;

        [[nodiscard]]
        HWND Handle() const;

        [[nodiscard]]
        Size WindowSize() const;

        void Destroy() const;
    };

    constexpr inline auto EngineWindow = EngineWindow_impl{};
}
