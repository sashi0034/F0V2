#pragma once
#include "StructuredBuffer.h"
#include "TextureResource.h"

namespace TY
{
    class ShaderResourceType : public Variant<TextureResource, StructuredBuffer>
    {
    public:
        using Variant::Variant;

        bool isEmpty() const
        {
            return isHolds<TextureResource>() && get<TextureResource>().isEmpty();
        }
    };

    class UnorderedAccessType : public Variant<UnorderedTextureResource, UnorderedStructuredBuffer>
    {
    public:
        using Variant::Variant;

        bool isEmpty() const
        {
            return isHolds<UnorderedTextureResource>() && get<UnorderedTextureResource>().isEmpty();
        }
    };
}
