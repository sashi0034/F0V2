#pragma once
#include "Value2D.h"

namespace TY
{
    struct ImageView
    {
        std::reference_wrapper<const uint8_t> dataReference;
        Size size;
        size_t sizeInBytes;
        DXGI_FORMAT format;

        ImageView(
            const uint8_t* dataPointer,
            const Size& size,
            size_t sizeInBytes,
            DXGI_FORMAT format)
            : dataReference(*dataPointer),
              size(size),
              sizeInBytes(sizeInBytes),
              format(format)
        {
        }

        const void* getPointer() const
        {
            return &dataReference.get();
        }

        int pixelSizeInBytes() const
        {
            return sizeInBytes / (size.x * size.y);
        }
    };
}
