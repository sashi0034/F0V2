#pragma once
#include "Array.h"
#include "Color.h"
#include "Value2D.h"

namespace TY
{
    class Image
    {
    public:
        Image() = default;

        Image(const Size& size);

        Image(const Size& size, const ColorU8& fillColor);

        ColorU8* operator[](int32_t y);

        const ColorU8* operator[](int32_t y) const;

        ColorU8& operator[](const Point& point);

        const ColorU8& operator[](const Point& point) const;

        const Size& size() const { return m_size; }

        size_t size_in_bytes() const { return m_size.x * m_size.y * sizeof(ColorU8); }

        const ColorU8* data() const { return m_data.data(); }

    private:
        Size m_size{};
        Array<ColorU8> m_data{};
    };
}
