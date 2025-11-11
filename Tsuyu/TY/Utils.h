#pragma once
#include <string>

#include "Array.h"

namespace TY
{
    [[nodiscard]]
    std::string ToLowercase(std::string_view str);

    [[nodiscard]]
    std::wstring ToUtf16(std::string_view str);

    [[nodiscard]]
    std::string ToUtf8(std::wstring_view wstr);

    [[nodiscard]]
    std::u32string ToUtf32(std::string_view str);

    [[nodiscard]]
    std::u32string ToUtf32(std::wstring_view wstr);

    template <class... Args>
    [[nodiscard]]
    std::u32string ToUtf32(std::format_string<Args...> fmt, Args&&... args)
    {
        return ToUtf32(std::format(fmt, std::forward<Args>(args)...));
    }

    template <class... Args>
    [[nodiscard]]
    std::u32string ToUtf32(std::wformat_string<Args...> fmt, Args&&... args)
    {
        return ToUtf32(std::format(fmt, std::forward<Args>(args)...));
    }

    [[nodiscard]]
    Array<std::string_view> SplitStringView(std::string_view str, char delimiter, bool skipEmpty = false);

    [[nodiscard]]
    std::wstring StringifyBlob(ID3DBlob* blob);

    /// @brief アライメントに揃えたサイズを取得する
    [[nodiscard]]
    constexpr size_t AlignedSize(size_t size, size_t alignment)
    {
        return size + alignment - (size % alignment);
    }
}
