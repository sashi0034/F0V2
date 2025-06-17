#pragma once
#include "Image.h"
#include "Variant.h"

namespace TY
{
    struct TextureSource : Variant<std::string, ImageView, ID3D12Resource*>
    {
        using Variant::Variant;
    };
}
