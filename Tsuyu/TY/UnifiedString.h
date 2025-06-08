#pragma once

namespace TY
{
    class UnifiedString
    {
    public:
        using utf8_string = std::string;
        using utf16_string = std::wstring;
        using variant_type = std::variant<utf8_string, utf16_string>;

        UnifiedString() = default;

        UnifiedString(const char* s) : m_data(utf8_string(s))
        {
        }

        UnifiedString(const wchar_t* ws) : m_data(utf16_string(ws))
        {
        }

        UnifiedString(const utf8_string& s) : m_data(s)
        {
        }

        UnifiedString(const utf16_string& ws) : m_data(ws)
        {
        }

        bool hasUtf8() const;

        bool hasUtf16() const;

        const utf8_string* getIfUtf8() const;

        const utf16_string* getIfUtf16() const;

        utf8_string toUtf8() const;

        utf16_string toUtf16() const;

        operator utf8_string() const;

        operator utf16_string() const;

    private:
        variant_type m_data{};
    };
}
