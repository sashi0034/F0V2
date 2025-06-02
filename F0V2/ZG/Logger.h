#pragma once
#include <string>

namespace ZG
{
    enum class LoggerKind : uint8_t
    {
        Info,
        Warning,
        Error,
        // Trace,
    };

    class Logger_impl
    {
    public:
        constexpr Logger_impl(LoggerKind kind) : m_kind(kind) { return; }

        /// @brief Write a horizontal rule
        const Logger_impl& HR() const;

        void Writeln(const std::wstring& message) const;

        const Logger_impl& operator <<(const std::wstring& message) const;

    private:
        LoggerKind m_kind;
    };

    static inline constexpr auto LogInfo = Logger_impl{LoggerKind::Info};

    static inline constexpr auto LogWarning = Logger_impl{LoggerKind::Warning};

    static inline constexpr auto LogError = Logger_impl{LoggerKind::Error};
}
