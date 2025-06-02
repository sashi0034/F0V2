#include "pch.h"
#include "Logger.h"

#include "AssertObject.h"

using namespace ZG;

namespace
{
    bool s_initializedConsole = false;

    // bool s_flushRequest{};

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

    void writelnInternal(const std::wstring& message, bool shouldStdOut)
    {
        // if (s_flushRequest)
        // {
        //     s_flushRequest = false;
        //     std::wcout << std::endl;
        // }

        // s_flushRequest = true;

        OutputDebugString((message + L"\n").c_str());

        if (shouldStdOut)
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

            std::wcout << message << std::endl;
        }
    }
}

namespace ZG
{
    const Logger_impl& Logger_impl::HR() const
    {
        writelnInternal(L"--------------------------------------------------", shouldStdOut(m_kind));
        return *this;
    }

    void Logger_impl::Writeln(const std::wstring& message) const
    {
        writelnInternal(getLoggerTag(m_kind) + message, shouldStdOut(m_kind));
    }

    const Logger_impl& Logger_impl::operator<<(const std::wstring& message) const
    {
        Writeln(message);
        return *this;
    }
}
