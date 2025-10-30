#pragma once

#include "UnifiedString.h"

namespace TY
{
    enum class LoggerKind : uint8_t
    {
        Info,
        Warning,
        Error,
        // Trace,
    };

    class LoggerSource_impl
    {
    public:
        using id_type = uint8_t;

        constexpr LoggerSource_impl(id_type id) : m_id(id)
        {
        }

        uint8_t id() const { return m_id; }

        void enableDebuggerOutput(bool enable) const;

        void enableConsoleOutput(bool enable) const;

    private:
        id_type m_id;
    };

    namespace LoggerSource
    {
        constexpr LoggerSource_impl Engine{0};

        constexpr LoggerSource_impl Title{1};
    }

    class Logger_impl
    {
    public:
        constexpr Logger_impl(LoggerKind kind, LoggerSource_impl source)
            : m_kind(kind), m_source(source)
        {
        }

        /// @brief Write a horizontal rule
        const Logger_impl& hr() const;

        void writeln(const UnifiedString& message) const;

        void operator()(const UnifiedString& message) const
        {
            writeln(message);
        }

        template <class... Args>
        void operator()(std::format_string<Args...> fmt, Args&&... args) const
        {
            writeln(std::format(fmt, std::forward<Args>(args)...));
        }

        template <class... Args>
        void operator()(std::wformat_string<Args...> fmt, Args&&... args) const
        {
            writeln(std::format(fmt, std::forward<Args>(args)...));
        }

        const Logger_impl& operator <<(const UnifiedString& message) const;

    private:
        LoggerKind m_kind;
        LoggerSource_impl m_source;
    };

    constexpr auto DefaultLoggerSource =
#if defined(TY_LIBRARY_BUILD)
        LoggerSource::Engine;
#else
    LoggerSource::Title;
#endif

    constexpr auto LogInfo = Logger_impl{LoggerKind::Info, DefaultLoggerSource};

    constexpr auto LogWarning = Logger_impl{LoggerKind::Warning, DefaultLoggerSource};

    constexpr auto LogError = Logger_impl{LoggerKind::Error, DefaultLoggerSource};
}
