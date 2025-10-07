#pragma once
#include "Immediate2D.h"
#include "ShapeBufferCreator.h"
#include "Vector2D.h"
#include "Vector4D.h"

namespace TY
{
    namespace ShapeBuilder2D
    {
        struct Vertex2D
        {
            Float2 pos;
            Float2 tex;
            Float4 color;

            void set(const Float2& pos_, const ColorF32& color_);

            void set(const Float2& pos_, const Float2& tex_, const ColorF32& color_);
        };

        using BufferCreator = ShapeBufferCreator<Vertex2D>;

        using index_type = BufferCreator::index_type;

        // -----------------------------------------------

        index_type BuildRect(BufferCreator& bufferCreator, const Immediate2D::Rect& rect);

        index_type BuildRoundRect(BufferCreator& bufferCreator, const Immediate2D::RoundRect& rect);

        index_type BuildLine(BufferCreator& bufferCreator, const Immediate2D::Line& line);

        index_type BuildSquareDotLine(BufferCreator& bufferCreator, const Immediate2D::SquareDotLine& line, float scale);

        index_type BuildPath(BufferCreator& bufferCreator, const Immediate2D::Path& path);

        index_type BuildCyclePath(BufferCreator& bufferCreator, const Immediate2D::CyclePath& cyclePath);

        index_type BuildText(BufferCreator& bufferCreator, const Immediate2D::Text& text);
    }
}
