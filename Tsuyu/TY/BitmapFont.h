#pragma once
#include "GlyphInfo.h"
#include "Grid.h"
#include "TextureResource.h"

namespace TY
{
    struct BitmapFontOptions
    {
        int atlasSize{1024};
    };

    class BitmapFont
    {
    public:
        BitmapFont() = default;

        BitmapFont(const std::string& filepath, int fontSize, const BitmapFontOptions& options = {});

        const GlyphInfo& fetchByCodePoint(char32_t codePoint) const;

        Array<GlyphInfo> fetchByString(const std::u32string& str) const;

        int fontSize() const;

        const Grid<uint8_t>& atlasImage() const;

        TextureResource atlasTexture() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
