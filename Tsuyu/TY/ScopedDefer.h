#pragma once

namespace TY
{
    class ScopedDefer
    {
    public:
        ScopedDefer() = default;

        explicit ScopedDefer(const std::function<void()>& func);

        ScopedDefer(const ScopedDefer&) = delete;

        ScopedDefer& operator=(const ScopedDefer&) = delete;

        ScopedDefer(ScopedDefer&& other) noexcept;

        ScopedDefer& operator=(ScopedDefer&& other) noexcept;

        ~ScopedDefer();

        void dispose();

    private:
        std::function<void()> m_func;
        bool m_active;
    };
}
