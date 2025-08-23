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

    class UnorderedAccessType : public UnorderedStructuredBuffer
    {
    public:
        using UnorderedStructuredBuffer::UnorderedStructuredBuffer;

        UnorderedAccessType(const UnorderedStructuredBuffer& other)
            : UnorderedStructuredBuffer(other)
        {
        }

        // TODO: using Variant::Variant;
    };
}
