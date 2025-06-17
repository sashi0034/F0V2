#include "pch.h"
#include "Logger.h"

#include "AssertObject.h"

using namespace TY;

namespace
{
    bool s_initializedConsole = false;

    struct LoggerSourceState
    {
        bool debuggerOutput{};
        bool consoleOutput{};
    };

    std::array<LoggerSourceState, 256> s_sourceStates{
        LoggerSourceState{true, false}, // Engine
        LoggerSourceState{true, true}, // Title
    };

    std::wstring getLoggerEmoji(LoggerKind kind)
    {
        static const std::vector<std::wstring> tags{
            L"ℹ️",
            L"⚠️",
            L"❌",
        };

        return tags[static_cast<int>(kind)];
    }

    std::wstring getLoggerTag(LoggerKind kind)
    {
        static const std::vector<std::wstring> tags{
            L"[info]    ",
            L"[warning] ",
            L"[error]   ",
        };

        return tags[static_cast<int>(kind)];
    }

    void ensureInitializedConsole()
    {
        if (s_initializedConsole) return;
        // -----------------------------------------------

        if (AllocConsole())
        {
            FILE* fp = nullptr;
            freopen_s(&fp, "CONOUT$", "w", stdout);
        }

        s_initializedConsole = true;
    }

    void writelnInternal(const std::wstring& message, bool hasTag, LoggerKind kind, LoggerSource_impl source)
    {
        ensureInitializedConsole();

        if (s_sourceStates[source.id()].debuggerOutput)
        {
            std::wstring debugString{};
            if (hasTag)
            {
                debugString = getLoggerEmoji(kind) + L" ";
            }

            debugString += message + L"\n";

            OutputDebugString(debugString.c_str());
        }

        if (s_sourceStates[source.id()].consoleOutput)
        {
            if (hasTag) std::wcout << getLoggerTag(kind);
            std::wcout << message << std::endl;
        }
    }
}

namespace TY
{
    void LoggerSource_impl::enableDebuggerOutput(bool enable) const
    {
        s_sourceStates[id()].debuggerOutput = enable;
    }

    void LoggerSource_impl::enableConsoleOutput(bool enable) const
    {
        s_sourceStates[id()].consoleOutput = enable;
    }

    const Logger_impl& Logger_impl::hr() const
    {
        writelnInternal(L"--------------------------------------------------", false, m_kind, m_source);
        return *this;
    }

    void Logger_impl::writeln(const UnifiedString& message) const
    {
        writelnInternal(message, true, m_kind, m_source);
    }

    const Logger_impl& Logger_impl::operator<<(const UnifiedString& message) const
    {
        writeln(message);
        return *this;
    }
}
