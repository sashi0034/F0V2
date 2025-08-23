#pragma once
#include "GlyphInfo.h"
#include "Grid.h"
#include "TextureResource.h"

namespace TY
{
    class BitmapFont
    {
    public:
        BitmapFont() = default;

        BitmapFont(const std::string& filepath, int fontSize);

        const GlyphInfo& fetchByCodePoint(char32_t codePoint) const;

        const Grid<uint8_t>& atlasImage() const;

        TextureResource fetchAtlasSrv() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
