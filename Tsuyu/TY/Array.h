#pragma once

namespace TY
{
    template <typename T>
    class Array : public std::vector<T>
    {
    public:
        using type = T;

        using std::vector<T>::vector;

        /// @brief バッファのサイズをバイト単位で取得
        [[nodiscard]]
        std::size_t size_in_bytes() const
        {
            return this->size() * sizeof(T);
        }
    };
}
