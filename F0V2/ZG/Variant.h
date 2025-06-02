#pragma once

namespace ZG
{
    // std::variant の拡張
    template <typename... Types>
    class Variant : public std::variant<Types...>
    {
    public:
        using std::variant<Types...>::variant;

        /// @brief 指定された型が現在保持されているか確認する
        template <typename T>
        bool isHolds() const
        {
            return std::holds_alternative<T>(*this);
        }

        /// @brief 指定された型の値を取得する
        template <typename T>
        T& get()
        {
            if (not std::holds_alternative<T>(*this))
            {
                throw std::bad_variant_access();
            }

            return std::get<T>(*this);
        }

        template <typename T>
        const T& get() const
        {
            if (not std::holds_alternative<T>(*this))
            {
                throw std::bad_variant_access();
            }

            return std::get<T>(*this);
        }

        template <typename T>
        T* tryGet()
        {
            if (not std::holds_alternative<T>(*this)) return nullptr;

            return &std::get<T>(*this);
        }

        template <typename T>
        const T* tryGet() const
        {
            if (not std::holds_alternative<T>(*this)) return nullptr;

            return &std::get<T>(*this);
        }
    };
}
