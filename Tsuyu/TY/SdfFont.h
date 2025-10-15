#pragma once
#include "GlyphInfo.h"
#include "Grid.h"
#include "TextureObject.h"

namespace TY
{
    struct SdfFontOptions
    {
        int atlasPadding{1};
        int atlasSize{2048};
        int sdfMargin{8};
    };

    class SdfFont
    {
    public:
        SdfFont() = default;

        SdfFont(const std::string& filepath, int fontSize, const SdfFontOptions& options = {});

        const GlyphInfo& fetchByCodePoint(char32_t codePoint) const;

        Array<GlyphInfo> fetchByString(const std::u32string& str) const;

        int fontSize() const;

        TextureObject atlasTexture() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
