#pragma once

namespace TY
{
    /// @brief コピー禁止 Mixin | Non-copyable mixin
    /// @remark このクラスを private 継承して使います。
    /// @remark Intended to be used as a private base class.
    class Uncopyable
    {
    protected:
        Uncopyable() = default;

        ~Uncopyable() = default;

        Uncopyable(const Uncopyable&) = delete;

        Uncopyable& operator =(const Uncopyable&) = delete;
    };
}
