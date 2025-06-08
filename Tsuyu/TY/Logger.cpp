#include "pch.h"
#include "Logger.h"

#include "AssertObject.h"

using namespace TY;

namespace
{
    bool s_initializedConsole = false;

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

    bool shouldStdOut(LoggerKind kind)
    {
#ifdef _DEBUG
        return true;
        // return kind == LoggerKind::Warning || kind == LoggerKind::Error;
#else
        return false;
#endif
    }

    void writelnInternal(const std::wstring& message, LoggerKind kind, bool hasTag)
    {
        std::wstring debugString{};
        if (hasTag)
        {
            debugString = getLoggerEmoji(kind) + L" ";
        }

        debugString += message + L"\n";

        OutputDebugString(debugString.c_str());

        if (shouldStdOut(kind))
        {
            if (not s_initializedConsole)
            {
                s_initializedConsole = true;

                if (AllocConsole())
                {
                    FILE* fp = nullptr;
                    freopen_s(&fp, "CONOUT$", "w", stdout);
                }
            }

            if (hasTag) std::wcout << getLoggerTag(kind);
            std::wcout << message << std::endl;
        }
    }
}

namespace TY
{
    const Logger_impl& Logger_impl::hr() const
    {
        writelnInternal(L"--------------------------------------------------", m_kind, false);
        return *this;
    }

    void Logger_impl::writeln(const std::wstring& message) const
    {
        writelnInternal(message, m_kind, true);
    }

    const Logger_impl& Logger_impl::operator<<(const std::wstring& message) const
    {
        writeln(message);
        return *this;
    }
}
