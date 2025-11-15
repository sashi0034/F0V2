#include "pch.h"
#include <tlhelp32.h>

#include "ProcessUtils.h"

namespace TY
{
    bool ProcessUtils::IsRunning(const UnifiedString& processName)
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        bool found = false;

        const std::wstring processName_ = processName.toUtf16();
        if (Process32FirstW(snap, &entry))
        {
            do
            {
                std::wstring_view exe = entry.szExeFile;

                if (exe == processName_)
                {
                    found = true;
                    break;
                }
            }
            while (Process32NextW(snap, &entry));
        }

        CloseHandle(snap);
        return found;
    }
}
