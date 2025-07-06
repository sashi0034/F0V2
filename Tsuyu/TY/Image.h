#pragma once
#include "Array.h"
#include "Color.h"
#include "ImageView.h"
#include "Vector2D.h"

namespace TY
{
    class Image
    {
    public:
        Image() = default;

        Image(const Size& size);

        Image(const Size& size, const ColorU8& fillColor);

        void fill(const ColorU8& colorU8);

        ColorU8* operator[](int32_t y);

        const ColorU8* operator[](int32_t y) const;

        ColorU8& operator[](const Point& point);

        const ColorU8& operator[](const Point& point) const;

        bool inBounds(const Point& point) const;

        const Size& size() const { return m_size; }

        size_t size_in_bytes() const { return m_size.x * m_size.y * sizeof(ColorU8); }

        Array<ColorU8>& data() { return m_data; }

        const Array<ColorU8>& data() const { return m_data; }

        ImageView view() const
        {
            return ImageView{
                reinterpret_cast<const uint8_t*>(m_data.data()),
                m_size,
                size_in_bytes(),
                DXGI_FORMAT_R8G8B8A8_UNORM
            };
        }

        operator ImageView() const
        {
            return view();
        }

    private:
        Size m_size{};
        Array<ColorU8> m_data{};
    };
}
