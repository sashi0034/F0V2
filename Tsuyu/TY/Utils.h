#pragma once
#include <string>

namespace TY
{
    std::wstring ToUtf16(std::string_view str);

    std::string ToUtf8(std::wstring_view wstr);

    std::u32string ToUtf32(std::string_view str);

    std::u32string ToUtf32(std::wstring_view wstr);

    template <class... Args>
    std::u32string ToUtf32(std::format_string<Args...> fmt, Args&&... args)
    {
        return ToUtf32(std::format(fmt, std::forward<Args>(args)...));
    }

    template <class... Args>
    std::u32string ToUtf32(std::wformat_string<Args...> fmt, Args&&... args)
    {
        return ToUtf32(std::format(fmt, std::forward<Args>(args)...));
    }

    std::wstring StringifyBlob(ID3DBlob* blob);

    /// @brief アライメントに揃えたサイズを取得する
    constexpr size_t AlignedSize(size_t size, size_t alignment)
    {
        return size + alignment - (size % alignment);
    }
}
