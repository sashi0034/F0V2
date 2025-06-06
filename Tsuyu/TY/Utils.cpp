#include "pch.h"
#include "Utils.h"

#include "AssertObject.h"

namespace TY
{
    std::wstring ToUtf16(const std::string& str)
    {
        // Get the required buffer size for the wide string
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);

        // Create a buffer to hold the wide string
        std::wstring wstr(size_needed, 0);

        // Perform the conversion
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr.data(), size_needed);

        // Remove the null terminator added by MultiByteToWideChar
        wstr.resize(size_needed - 1);

        return wstr;
    }

    std::string ToUtf8(const std::wstring& wstr)
    {
        // Get the required buffer size for the UTF-8 string (including null terminator)
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);

        // Create a buffer to hold the UTF-8 string
        std::string str(size_needed, 0);

        // Perform the conversion from UTF-16 to UTF-8
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size_needed, nullptr, nullptr);

        // Remove the null terminator added by WideCharToMultiByte
        str.resize(size_needed - 1);

        return str;
    }

    std::wstring StringifyBlob(ID3DBlob* blob)
    {
        return ToUtf16(std::string{static_cast<char*>(blob->GetBufferPointer()), blob->GetBufferSize()});
    }
}
