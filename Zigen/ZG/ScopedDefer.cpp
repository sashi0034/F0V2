#include "pch.h"
#include "ScopedDefer.h"

namespace ZG
{
    ScopedDefer::ScopedDefer(const std::function<void()>& func): m_func(func), m_active(true)
    {
    }

    ScopedDefer::ScopedDefer(ScopedDefer&& other) noexcept: m_func(std::move(other.m_func)), m_active(other.m_active)
    {
        other.m_active = false;
    }

    ScopedDefer& ScopedDefer::operator=(ScopedDefer&& other) noexcept
    {
        if (this != &other)
        {
            m_func = std::move(other.m_func);
            m_active = other.m_active;
            other.m_active = false;
        }

        return *this;
    }

    ScopedDefer::~ScopedDefer()
    {
        if (m_active)
        {
            if (m_func) m_func();
        }
    }

    void ScopedDefer::dispose()
    {
        if (m_active)
        {
            if (m_func) m_func();
            m_active = false;
        }
    }
}
