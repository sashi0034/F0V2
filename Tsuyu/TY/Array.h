#pragma once

namespace TY
{
    template <typename Type>
    class Array : public std::vector<Type>
    {
    public:
        using type = Type;

        using std::vector<Type>::vector;

        int indexOf(const Type& value) const
        {
            for (size_t i = 0; i < this->size(); ++i)
            {
                if ((*this)[i] == value)
                {
                    return static_cast<int>(i);
                }
            }

            return -1;
        }

        template <class Fty, std::enable_if_t<std::is_invocable_r_v<bool, Fty, Type>>* = nullptr>
        int indexOf(Fty f) const
        {
            for (size_t i = 0; i < this->size(); ++i)
            {
                if (f((*this)[i]))
                {
                    return static_cast<int>(i);
                }
            }

            return -1;
        }

        /// @brief バッファのサイズをバイト単位で取得
        [[nodiscard]]
        std::size_t size_in_bytes() const
        {
            return this->size() * sizeof(Type);
        }
    };
}
