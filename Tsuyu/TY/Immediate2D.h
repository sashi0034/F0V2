#pragma once
#include "Alignment.h"
#include "Array.h"
#include "Color.h"
#include "FontObject.h"
#include "Rect.h"
#include "Variant.h"

namespace TY
{
    namespace Immediate2D
    {
        struct Outline
        {
            float thickness;
            ColorF32 innerColor;
            ColorF32 outerColor;

            Outline() = default;

            Outline(float thickness, ColorF32 color);

            Outline(float thickness, ColorF32 innerColor, ColorF32 outerColor);
        };

        struct Rect
        {
            RectF rect;
            std::array<ColorF32, 4> colors = {ColorF32{1.0}, ColorF32{1.0}, ColorF32{1.0}, ColorF32{1.0}};
            Outline outline{};

            Rect() = default;

            Rect(const RectF& rect_);

            Rect& setColor(const ColorF32& color_);

            Rect& setOutline(const Outline& outline_);

            void pushAuto();
        };

        struct RoundRect
        {
            RectF rect;
            ColorF32 color{ColorF32{1.0f}};
            float roundness{5.0f};
            int segments{2};
            Outline outline{};

            RoundRect() = default;

            RoundRect(const RectF& rect);

            RoundRect& setColor(const ColorF32& color_);

            RoundRect& setRoundness(float roundness_, int segments_ = 2);

            RoundRect& setOutline(const Outline& outline_);

            void pushAuto();
        };

        struct SquareDotLine;

        struct Line
        {
            Float2 start;
            Float2 end;
            float thickness{1.0f};
            std::array<ColorF32, 2> colors = {ColorF32{1.0}, ColorF32{1.0}};

            Line() = default;

            Line(const Float2& start_, const Float2& end_);

            Line(float x1, float y1, float x2, float y2);

            Line& setThickness(float thickness_);

            Line& setColor(const ColorF32& color_);

            SquareDotLine asDotLine(float dotOffset = 0.0f) const;

            void pushAuto();
        };

        struct SquareDotLine
        {
            Line line;
            float dotOffset;

            SquareDotLine& setDotOffset(float offset_);

            void pushAuto();
        };

        struct CyclePath;

        struct Path
        {
            Array<Float2> points;
            float thickness{1.0f};
            ColorF32 color{ColorF32{1.0}};

            Path() = default;

            Path(const Array<Float2>& points_);

            Path& append(const Float2& p);

            Path& setThickness(float thickness_);

            Path& setColor(const ColorF32& color_);

            CyclePath asCycle();

            void pushAuto();
        };

        struct CyclePath
        {
            Path path;

            CyclePath() = default;

            CyclePath(Path path_);

            void pushAuto();
        };

        struct Texture
        {
            TextureHandle texture;
            Float2 position{};
            Float2 pivot{};
            Float2 scale{1.0f, 1.0f};
            RectF uvRect{0.0f, 0.0f, 1.0f, 1.0f};
            ColorF32 color{1.0};

            Texture() = default;

            Texture(const TextureHandle& handle_);

            Texture& setPosition(const Float2& position_, Alignment9 alignment = Alignment9::TopLeft);

            Texture& setScale(const Float2& scale_);

            Texture& resized(const Float2& size);

            void pushAuto();
        };

        struct CachedText;

        struct Text
        {
            FontObject font;
            std::u32string text;
            std::optional<float> size{};
            Float2 position{0.0f, 0.0f};
            Float2 pivot{};
            ColorF32 color{ColorF32{1.0}};

            Text() = default;

            Text(const FontObject& font_, const std::u32string& text_);

            Text& setSize(float size_);

            Text& setPosition(const Float2& position_, Alignment9 alignment = Alignment9::TopLeft);

            Text& setColor(const ColorF32& color_);

            struct build_intermediate
            {
                Float2 posTL;
                Float2 posBR;
                Float2 uvTL;
                Float2 uvBR;
            };

            RectF build(const std::function<void(build_intermediate)>& callback, Float2& offsetToApply) const;

            CachedText cache() const;

            void pushAuto();
        };

        struct CachedText
        {
            FontObject font;

            ColorF32 color{ColorF32{1.0}};

            struct character_type
            {
                Float2 posTL;
                Float2 posBR;
                Float2 uvTL;
                Float2 uvBR;
            };

            Array<character_type> characters{};

            RectF region{};

            void pushAuto();
        };

        // -----------------------------------------------

        using shape_type = Variant<
            Rect,
            RoundRect,
            Line,
            SquareDotLine,
            Path,
            CyclePath,
            Texture,
            Text,
            CachedText
        >;
    }
}
