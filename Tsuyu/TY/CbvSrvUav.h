#pragma once

#include "DepthStencilHandle.h"
#include "StructuredBuffer.h"
#include "TextureHandle.h"
#include "Variant.h"

namespace TY
{
    class ShaderResourceType : public Variant<TextureHandle, StructuredBufferObject, DepthBufferHandle>
    {
    public:
        using Variant::Variant;

        [[nodiscard]]
        bool isEmpty() const
        {
            return std::visit([](const auto& res) { return res.isEmpty(); }, *this);
        }
    };

    class UnorderedAccessType : public Variant<UnorderedTextureHandle, UnorderedStructuredBufferObject>
    {
    public:
        using Variant::Variant;

        [[nodiscard]]
        bool isEmpty() const
        {
            return std::visit([](const auto& res) { return res.isEmpty(); }, *this);
        }
    };
}
