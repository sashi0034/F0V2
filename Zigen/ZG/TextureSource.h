#pragma once
#include "Image.h"
#include "Variant.h"

namespace ZG
{
    struct TextureSource : Variant<std::string, Image, ID3D12Resource*>
    {
        using Variant::Variant;
    };
}
