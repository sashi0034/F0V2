#include "pch.h"
#include "Utils.h"

#include "AssertObject.h"

namespace TY
{
    std::string ToLowercase(std::string_view str)
    {
        std::string result;
        result.reserve(str.size());

        for (char c : str)
        {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        return result;
    }

    std::wstring ToUtf16(std::string_view str)
    {
        // Get the required buffer size for the wide string
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, nullptr, 0);

        // Create a buffer to hold the wide string
        std::wstring wstr(size_needed, 0);

        // Perform the conversion
        MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, wstr.data(), size_needed);

        // Remove the null terminator added by MultiByteToWideChar
        wstr.resize(size_needed - 1);

        return wstr;
    }

    std::string ToUtf8(std::wstring_view wstr)
    {
        // Get the required buffer size for the UTF-8 string (including null terminator)
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, nullptr, 0, nullptr, nullptr);

        // Create a buffer to hold the UTF-8 string
        std::string str(size_needed, 0);

        // Perform the conversion from UTF-16 to UTF-8
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, &str[0], size_needed, nullptr, nullptr);

        // Remove the null terminator added by WideCharToMultiByte
        str.resize(size_needed - 1);

        return str;
    }

    namespace
    {
        inline constexpr char32_t k_replacementChar = U'\uFFFD';
    }

    // std::u32string ToUtf32(const std::string& str)
    // {
    //     return ToUtf32(std::string_view{str});
    // }

    std::u32string ToUtf32(std::string_view str)
    {
        // UTF-8 decoder with validation. Errors -> U+FFFD.
        std::u32string out;
        out.reserve(str.size()); // worst-case: 1 byte -> 1 code point

        const auto len = str.size();
        size_t i = 0;

        while (i < len)
        {
            unsigned char c0 = static_cast<unsigned char>(str[i]);

            // 1-byte (ASCII)
            if (c0 <= 0x7F)
            {
                out.push_back(static_cast<char32_t>(c0));
                ++i;
                continue;
            }

            int needed = 0;
            char32_t cp = 0;

            if ((c0 & 0xE0) == 0xC0)
            {
                needed = 2;
                cp = c0 & 0x1F;
            }
            else if ((c0 & 0xF0) == 0xE0)
            {
                needed = 3;
                cp = c0 & 0x0F;
            }
            else if ((c0 & 0xF8) == 0xF0)
            {
                needed = 4;
                cp = c0 & 0x07;
            }
            else
            {
                // Invalid leading byte
                out.push_back(k_replacementChar);
                ++i;
                continue;
            }

            if (i + needed > len)
            {
                // Truncated sequence at end
                out.push_back(k_replacementChar);
                break;
            }

            bool ok = true;
            for (int j = 1; j < needed; ++j)
            {
                unsigned char cx = static_cast<unsigned char>(str[i + j]);
                if ((cx & 0xC0) != 0x80)
                {
                    ok = false;
                    // consume the invalid lead only; let the next loop re-evaluate following bytes
                    out.push_back(k_replacementChar);
                    ++i;
                    break;
                }
                cp = (cp << 6) | (cx & 0x3F);
            }
            if (!ok) continue;

            // Overlong-sequence checks and range checks
            if ((needed == 2 && cp < 0x80) ||
                (needed == 3 && cp < 0x800) ||
                (needed == 4 && cp < 0x10000) ||
                cp > 0x10FFFF ||
                (cp >= 0xD800 && cp <= 0xDFFF)) // UTF-16 surrogate range not valid in Unicode scalar values
            {
                out.push_back(k_replacementChar);
                i += needed;
                continue;
            }

            out.push_back(cp);
            i += needed;
        }

        return out;
    }

    // std::u32string ToUtf32(const std::wstring& wstr)
    // {
    //     return ToUtf32(std::wstring_view{wstr.data(), wstr.size()});
    // }

    std::u32string ToUtf32(std::wstring_view wstr)
    {
        // Portable: wchar_t could be 16-bit (Windows, UTF-16) or 32-bit (Linux, UTF-32)
        std::u32string result;
        result.reserve(wstr.size());

#if WCHAR_MAX <= 0xFFFF
        // UTF-16 input (typical on Windows)
        for (size_t i = 0; i < wstr.size(); ++i)
        {
            const wchar_t wc = wstr[i];

            // High surrogate
            if (wc >= 0xD800 && wc <= 0xDBFF)
            {
                if (i + 1 < wstr.size())
                {
                    const wchar_t low = wstr[i + 1];
                    if (low >= 0xDC00 && low <= 0xDFFF)
                    {
                        const char32_t cp = (static_cast<char32_t>(wc - 0xD800) << 10)
                            + (static_cast<char32_t>(low - 0xDC00))
                            + 0x10000;
                        result.push_back(cp);
                        ++i;
                        continue;
                    }
                }
                // Lone high surrogate -> replacement
                result.push_back(k_replacementChar);
                continue;
            }

            // Lone low surrogate -> replacement
            if (wc >= 0xDC00 && wc <= 0xDFFF)
            {
                result.push_back(k_replacementChar);
                continue;
            }

            result.push_back(static_cast<char32_t>(wc));
        }
#else
        // UTF-32 input (typical on Linux)
        for (wchar_t wc : wstr)
        {
            char32_t cp = static_cast<char32_t>(wc);
            if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            {
                result.push_back(k_replacementChar);
            }
            else
            {
                result.push_back(cp);
            }
        }
#endif

        return result;
    }

    Array<std::string_view> SplitStringView(std::string_view str, char delimiter, bool skipEmpty)
    {
        Array<std::string_view> result;

        size_t start = 0;
        while (start <= str.size())
        {
            size_t end = str.find(delimiter, start);
            if (end == std::string_view::npos)
            {
                end = str.size();
            }

            std::string_view token = str.substr(start, end - start);
            if (not(skipEmpty && token.empty()))
            {
                result.push_back(token);
            }

            start = end + 1;
        }

        return result;
    }

    std::wstring StringifyBlob(ID3DBlob* blob)
    {
        return ToUtf16(std::string{static_cast<char*>(blob->GetBufferPointer()), blob->GetBufferSize()});
    }
}
