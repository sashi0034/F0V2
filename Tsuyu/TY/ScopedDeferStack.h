#pragma once
#include "ScopedDefer.h"

namespace TY
{
    class ScopedDeferStack
    {
    public:
        ScopedDeferStack() = default;

        ScopedDeferStack(const ScopedDeferStack&) = delete;

        ScopedDeferStack& operator=(const ScopedDeferStack&) = delete;

        ScopedDeferStack(ScopedDeferStack&& other) noexcept;

        ScopedDeferStack& operator=(ScopedDeferStack&& other) noexcept;

        ~ScopedDeferStack();

        void dispose();

        ScopedDeferStack push(ScopedDefer&& disposable);

        template <typename... Ts>
        ScopedDeferStack push(ScopedDefer&& first, Ts&&... rest)
        {
            auto p = push(std::move(first));
            ((p = p.push(std::move(rest))), ...);
            return p;
        }

    private:
        std::vector<ScopedDefer> m_disposables;
        bool m_active;
    };
}
