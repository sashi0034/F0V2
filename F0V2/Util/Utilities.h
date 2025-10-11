#pragma once
#include "TY/Array.h"
#include "TY/Rect.h"

namespace Util
{
    Array<RectF> SliceRectByLength(const RectF& rect, float length, Direction2 dir);

    /// @brief enum の文字列を取得する
    /// @remarks enum の先頭は 0 から始まる必要がある
    template <typename Enum>
    [[nodiscard]]
    inline const Array<std::string>& GetEnumNameList(Enum maxValue)
    {
        static Array<std::string> names{};
        if (names.empty())
        {
            for (int i = 0; i < static_cast<int>(maxValue) + 1; ++i)
            {
                names.push_back(std::string(NAMEOF_ENUM((Enum)i)));
            }
        }

        return names;
    }

    /// @brief enum の文字列を取得する
    /// @remarks enum に最大値を表すメンバ Max が必要
    template <typename Enum>
    [[nodiscard]]
    inline const Array<std::string>& GetEnumNameList()
    {
        return GetEnumNameList(Enum::Max);
    }

    template <typename Enum>
    [[nodiscard]]
    inline const Array<const char*>& GetEnumCStrList()
    {
        static Array<const char*> cstrList{};
        if (cstrList.empty())
        {
            const auto& strList = GetEnumNameList<Enum>();
            cstrList.reserve(strList.size());
            for (auto& s : strList)
            {
                cstrList.push_back(s.c_str());
            }
        }

        return cstrList;
    }

    /// @brief enum の文字列を取得する
    template <typename Enum>
    [[nodiscard]]
    inline std::string GetEnumName(Enum value, Enum maxValue)
    {
        return GetEnumNames(value, maxValue)[static_cast<int>(value)];
    }

    /// @brief enum の文字列を取得する
    /// @remarks enum に最大値を表すメンバ Max が必要
    template <typename Enum>
    [[nodiscard]]
    inline std::string GetEnumName(Enum value)
    {
        return GetEnumNameList(Enum::Max)[static_cast<int>(value)];
    }

    /// @brief 文字列から enum の値を取得する
    template <typename Enum>
    [[nodiscard]]
    inline std::optional<Enum> GetEnumValueByName(const std::string& name, Enum maxValue)
    {
        static std::unordered_map<std::string, Enum> hashset{};
        if (hashset.size() == 0)
        {
            for (int i = 0; i < static_cast<int>(maxValue); ++i)
            {
                hashset.emplace(GetEnumNameList(maxValue)[i], static_cast<Enum>(i));
            }
        }

        if (hashset.contains(name))
        {
            return hashset[name];
        }

        return std::nullopt;
    }

    /// @brief 文字列から enum の値を取得する
    /// @remarks enum に最大値を表すメンバ Max が必要
    template <typename Enum>
    [[nodiscard]]
    inline std::optional<Enum> GetEnumValueByName(const std::string& name)
    {
        return GetEnumValueByName(name, Enum::Max);
    }
}
