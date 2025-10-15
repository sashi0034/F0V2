#pragma once
#include "StructuredBuffer.h"
#include "TextureObject.h"
#include "Variant.h"

namespace TY
{
    class ShaderResourceType : public Variant<TextureObject, StructuredBuffer>
    {
    public:
        using Variant::Variant;
    };

    class UnorderedAccessType : public Variant<UnorderedTextureObject, UnorderedStructuredBuffer>
    {
    public:
        using Variant::Variant;
    };
}
