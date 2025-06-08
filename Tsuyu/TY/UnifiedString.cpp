#include "pch.h"
#include "UnifiedString.h"

#include "Utils.h"

namespace TY
{
    bool UnifiedString::hasUtf8() const
    {
        return std::holds_alternative<utf16_string>(m_data);
    }

    bool UnifiedString::hasUtf16() const
    {
        return std::holds_alternative<utf8_string>(m_data);
    }

    const UnifiedString::utf8_string* UnifiedString::getIfUtf8() const
    {
        if (const auto p = std::get_if<utf8_string>(&m_data))
        {
            return p;
        }

        return nullptr;
    }

    const UnifiedString::utf16_string* UnifiedString::getIfUtf16() const
    {
        if (const auto p = std::get_if<utf16_string>(&m_data))
        {
            return p;
        }

        return nullptr;
    }

    UnifiedString::utf8_string UnifiedString::toUtf8() const
    {
        if (const auto p = std::get_if<utf8_string>(&m_data))
        {
            return *p;
        }
        else if (const auto p = std::get_if<utf16_string>(&m_data))
        {
            return ToUtf8(*p);
        }
        else
        {
            assert(false);
            return {};
        }
    }

    UnifiedString::utf16_string UnifiedString::toUtf16() const
    {
        if (const auto p = std::get_if<utf16_string>(&m_data))
        {
            return *p;
        }
        else if (const auto p = std::get_if<utf8_string>(&m_data))
        {
            return ToUtf16(*p);
        }
        else
        {
            assert(false);
            return {};
        }
    }

    UnifiedString::operator utf8_string() const
    {
        return toUtf8();
    }

    UnifiedString::operator utf16_string() const
    {
        return toUtf16();
    }
}
