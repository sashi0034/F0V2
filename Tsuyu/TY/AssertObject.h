#pragma once

#include <source_location>
#include <string>

#include "Windows.h"

namespace TY
{
    struct AssertObject
    {
        std::string_view errorMessage;
        int index{};
#ifdef _DEBUG
        std::source_location location{};

        AssertObject(
            std::string_view errorMessage,
            const std::source_location& location = std::source_location::current());
#else
        AssertObject(std::string_view errorMessage);
#endif

        [[noreturn]] void throwError() const;
    };

    struct AssertWin32 : AssertObject
    {
        AssertWin32 operator |(HRESULT result) const;
    };

    struct AssertNotNull : AssertObject
    {
        AssertNotNull operator |(const void* ptr) const;
    };

    struct AssertTrue : AssertObject
    {
        AssertTrue operator |(bool condition) const;
    };
}
