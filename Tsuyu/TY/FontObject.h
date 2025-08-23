#pragma once
#include "BitmapFont.h"
#include "SdfFont.h"
#include "Variant.h"

namespace TY
{
    class FontObject : public Variant<BitmapFont, SdfFont>
    {
        const GlyphInfo& fetchByCodePoint(char32_t codePoint) const
        {
            return std::visit([codePoint](const auto& font) -> const GlyphInfo&
            {
                return font.fetchByCodePoint(codePoint);
            }, *this);
        }

        int fontSize() const
        {
            return std::visit([](const auto& font) { return font.fontSize(); }, *this);
        }

        TextureResource atlasTexture() const
        {
            return std::visit([](const auto& font) { return font.atlasTexture(); }, *this);
        }
    };
}
