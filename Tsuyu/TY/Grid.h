#pragma once
#include "Array.h"
#include "Integer2D.h"

namespace TY
{
    template <class Type>
    class Grid
    {
    public:
        using container_type = Array<Type>;
        using value_type = Type;
        using size_type = size_t;

        Grid() = default;

        Grid(const Grid&) = default;

        Grid(Grid&&) = default;

        Grid& operator =(const Grid&) = default;

        Grid& operator =(Grid&&) = default;

        Grid(size_type w, size_t h)
            : m_width(w), m_height(h)
        {
            m_data.resize(w * h);
        }

        Grid(Size size)
            : m_width(size.x), m_height(size.y)
        {
            m_data.resize(size.x * size.y);
        }

        Size size() const
        {
            return Size{static_cast<int>(m_width), static_cast<int>(m_height)};
        }

        size_t size_in_bytes() const
        {
            return m_data.size() * sizeof(value_type);
        }

        size_type width() const
        {
            return m_width;
        }

        size_type height() const
        {
            return m_height;
        }

        bool isEmpty() const
        {
            return m_data.empty();
        }

        [[nodiscard]]
        value_type* operator [](size_t index)
        {
            return &m_data[index * m_width];
        }

        [[nodiscard]]
        const value_type* operator [](size_t index) const
        {
            return &m_data[index * m_width];
        }

        [[nodiscard]]
        value_type& operator [](Point pos)
        {
            return m_data[pos.y * m_width + pos.x];
        }

        [[nodiscard]]
        const value_type& operator [](Point pos) const
        {
            return m_data[pos.y * m_width + pos.x];
        }

        [[nodiscard]]
        value_type* data()
        {
            return m_data.data();
        }

        [[nodiscard]]
        const value_type* data() const
        {
            return m_data.data();
        }

    private:
        container_type m_data{};
        size_type m_width{};
        size_type m_height{};
    };
}
