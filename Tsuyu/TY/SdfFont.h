#pragma once
#include "GlyphInfo.h"
#include "Grid.h"
#include "TextureHandle.h"

namespace TY
{
    struct SdfFontOptions
    {
        int atlasPadding{1};
        int atlasSize{2048};
        int sdfMargin{8};

        /// @brief 数値の等幅量
        int tabularFigures{};
    };

    class SdfFont
    {
    public:
        SdfFont() = default;

        SdfFont(const std::string& filepath, int fontSize, const SdfFontOptions& options = {});

        /// @brief フォールバック込みでフォントを作成
        SdfFont(const Array<std::string>& filepaths, int fontSize, const SdfFontOptions& options = {});

        const GlyphInfo& fetchByCodePoint(char32_t codePoint) const;

        Array<GlyphInfo> fetchByString(const std::u32string& str) const;

        int fontSize() const;

        TextureHandle atlasTexture() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
