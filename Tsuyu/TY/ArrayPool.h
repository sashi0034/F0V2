#pragma once
#include "Array.h"

namespace TY
{
    template <typename T>
    class ArrayPool
    {
    private:
        Array<T> m_buffer{};
        size_t m_logical_size{};

    public:
        bool logical_empty() const
        {
            return m_logical_size == 0;
        }

        void logical_resize(size_t new_size)
        {
            if (new_size > m_buffer.size())
            {
                m_buffer.resize(new_size);
            }

            m_logical_size = new_size;
        }

        void add_logical_size(size_t new_size)
        {
            if (m_logical_size + new_size > m_buffer.size())
            {
                m_buffer.resize(m_logical_size + new_size);
            }

            m_logical_size += new_size;
        }

        size_t logical_size() const
        {
            return m_logical_size;
        }

        T& logical_back()
        {
            if (m_logical_size == 0)
            {
                throw std::out_of_range("ArrayPool: logical_back called on empty pool.");
            }

            return m_buffer[m_logical_size - 1];
        }

        const T& logical_back() const
        {
            if (m_logical_size == 0)
            {
                throw std::out_of_range("ArrayPool: logical_back called on empty pool.");
            }

            return m_buffer[m_logical_size - 1];
        }

        void push_logical_back(const T& item)
        {
            if (m_logical_size >= m_buffer.size())
            {
                m_buffer.resize(m_logical_size + 1);
            }

            m_buffer[m_logical_size] = item;
            ++m_logical_size;
        }

        T* data()
        {
            return m_buffer.data();
        }

        const T* data() const
        {
            return m_buffer.data();
        }

        T& operator[](size_t index)
        {
            if (index >= m_logical_size)
            {
                throw std::out_of_range("ArrayPool: Index out of range.");
            }

            return m_buffer[index];
        }

        const T& operator[](size_t index) const
        {
            if (index >= m_logical_size)
            {
                throw std::out_of_range("ArrayPool: Index out of range.");
            }

            return m_buffer[index];
        }

        // std::span<T> span()
        // {
        //     return std::span<T>(m_buffer.data(), m_logical_size);
        // }
        //
        // std::span<const T> span() const
        // {
        //     return std::span<const T>(m_buffer.data(), m_logical_size);
        // }

        const Array<T>& buffer() const
        {
            return m_buffer;
        }

        // -----------------------------------------------
        // Iterators

        using iterator = typename Array<T>::iterator;
        using const_iterator = typename Array<T>::const_iterator;

        iterator begin()
        {
            return m_buffer.begin();
        }

        iterator end()
        {
            return m_buffer.begin() + m_logical_size;
        }

        const_iterator begin() const
        {
            return m_buffer.begin();
        }

        const_iterator end() const
        {
            return m_buffer.begin() + m_logical_size;
        }

        const_iterator cbegin() const
        {
            return m_buffer.begin();
        }

        const_iterator cend() const
        {
            return m_buffer.begin() + m_logical_size;
        }
    };
}
