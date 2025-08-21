#pragma once
#include "DynamicTexture.h"
#include "GlyphInfo.h"
#include "Grid.h"

namespace TY
{
    class BitmapFont
    {
    public:
        BitmapFont() = default;

        BitmapFont(const std::string& filepath, int fontSize);

        const GlyphInfo& fetchByCodePoint(char32_t codePoint) const;

        const Grid<uint8_t>& atlasImage() const;

        DynamicTexture fetchAtlasTexture() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
