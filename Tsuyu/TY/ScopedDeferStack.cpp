#include "pch.h"
#include "ScopedDeferStack.h"

namespace TY
{
    ScopedDeferStack::ScopedDeferStack(ScopedDeferStack&& other) noexcept
        : m_disposables(std::move(other.m_disposables)),
          m_active(other.m_active)
    {
        other.m_active = false;
    }

    ScopedDeferStack& ScopedDeferStack::operator=(ScopedDeferStack&& other) noexcept
    {
        if (this != &other)
        {
            m_disposables = std::move(other.m_disposables);
            m_active = other.m_active;
            other.m_active = false;
        }

        return *this;
    }

    ScopedDeferStack::~ScopedDeferStack()
    {
        if (m_active)
        {
            dispose();
        }
    }

    void ScopedDeferStack::dispose()
    {
        if (!m_active) return;

        // 後入れ先出し順に dispose
        for (auto it = m_disposables.rbegin(); it != m_disposables.rend(); ++it)
        {
            it->dispose();
        }

        m_disposables.clear();
        m_active = false;
    }

    ScopedDeferStack ScopedDeferStack::push(ScopedDefer&& disposable)
    {
        m_disposables.emplace_back(std::move(disposable));
        m_active = true;
        return std::move(*this);
    }
}
