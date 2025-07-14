#pragma once
#include "ShaderResourceTexture.h"
#include "StructuredBufferUploader.h"

namespace TY
{
    class ShaderResourceType : public Variant<ShaderResourceTexture, StructuredBufferUploader>
    {
    public:
        using Variant::Variant;

        bool isEmpty() const
        {
            return isHolds<ShaderResourceTexture>() && get<ShaderResourceTexture>().isEmpty();
        }
    };

    class UnorderedAccessType : public StructuredBufferUploader
    {
    public:
        using StructuredBufferUploader::StructuredBufferUploader;

        UnorderedAccessType(const StructuredBufferUploader& other)
            : StructuredBufferUploader(other)
        {
        }

        // TODO: using Variant::Variant;
    };
}
