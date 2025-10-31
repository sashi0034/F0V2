#pragma once

inline namespace Util_inline
{
    template <typename T>
    class PositiveValue
    {
    public:
        explicit PositiveValue(T value = 0)
            : m_value(value < 0 ? 0 : value)
        {
        }

        operator T() const
        {
            return m_value;
        }

        // PositiveValue& operator=(T value)
        // {
        //     m_value = value < 0 ? 0 : value;
        //     return *this;
        // }

    private:
        T m_value;
    };

    using PositiveF32 = PositiveValue<float>;
}
