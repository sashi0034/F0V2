#pragma once
#include "TY/Alignment.h"

inline namespace Util_inline
{
    void InitImmediatePrintAddon();

    void ImmediatePrint(std::string_view message, Alignment9 align = Alignment9::TopLeft);

    void ImmediatePrint(const std::u32string& message, Alignment9 align = Alignment9::TopLeft);

    // -----------------------------------------------

    template <class... Args>
    void ImmediatePrint(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::TopLeft);
    }

    template <class... Args>
    void ImmediatePrint_TopLeft(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::TopLeft);
    }

    template <class... Args>
    void ImmediatePrint_TopCenter(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::TopCenter);
    }

    template <class... Args>
    void ImmediatePrint_TopRight(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::TopRight);
    }

    template <class... Args>
    void ImmediatePrint_MiddleLeft(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::MiddleLeft);
    }

    template <class... Args>
    void ImmediatePrint_MiddleCenter(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::MiddleCenter);
    }

    template <class... Args>
    void ImmediatePrint_MiddleRight(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::MiddleRight);
    }

    template <class... Args>
    void ImmediatePrint_BottomLeft(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::BottomLeft);
    }

    template <class... Args>
    void ImmediatePrint_BottomCenter(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::BottomCenter);
    }

    template <class... Args>
    void ImmediatePrint_BottomRight(std::format_string<Args...> fmt, Args&&... args)
    {
        ImmediatePrint(std::format(fmt, std::forward<Args>(args)...), Alignment9::BottomRight);
    }
}
