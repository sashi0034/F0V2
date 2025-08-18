#pragma once
#include "ShaderResourceTexture.h"
#include "StructuredBuffer.h"

namespace TY
{
    class ShaderResourceType : public Variant<ShaderResourceTexture, StructuredBuffer>
    {
    public:
        using Variant::Variant;

        bool isEmpty() const
        {
            return isHolds<ShaderResourceTexture>() && get<ShaderResourceTexture>().isEmpty();
        }
    };

    class UnorderedAccessType : public StructuredBuffer
    {
    public:
        using StructuredBuffer::StructuredBuffer;

        UnorderedAccessType(const StructuredBuffer& other)
            : StructuredBuffer(other)
        {
        }

        // TODO: using Variant::Variant;
    };
}
