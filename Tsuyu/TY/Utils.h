#pragma once
#include <string>

namespace TY
{
    std::wstring ToUtf16(const std::string& str);

    std::string ToUtf8(const std::wstring& wstr);

    std::wstring StringifyBlob(ID3DBlob* blob);

    /// @brief アライメントに揃えたサイズを取得する
    constexpr size_t AlignedSize(size_t size, size_t alignment)
    {
        return size + alignment - (size % alignment);
    }
}
