#pragma once
#include "Rect.h"
#include "Shader.h"
#include "TextureSource.h"

namespace TY
{
    struct TextureDrawerParams
    {
        TextureSource source;
        PixelShader ps;
        VertexShader vs;

        TextureDrawerParams& setSource(const TextureSource& source_)
        {
            source = source_;
            return *this;
        }

        TextureDrawerParams& setPS(const PixelShader& ps_)
        {
            ps = ps_;
            return *this;
        }

        TextureDrawerParams& setVS(const VertexShader& vs_)
        {
            vs = vs_;
            return *this;
        }
    };

    class TextureDrawer
    {
        friend struct TextureDrawable2D;

    public:
        TextureDrawer() = default;

        TextureDrawer(const TextureDrawerParams& params);

        TextureDrawer(const TextureSource& source, const PixelShader& ps, const VertexShader& vs)
            : TextureDrawer(TextureDrawerParams{source, ps, vs})
        {
        }

        [[nodiscard]]
        TextureDrawable2D as2D() const;

        void draw3D() const;

        [[nodiscard]]
        Size size() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
        std::wstring m_filename;
    };

    struct TextureDrawable2D
    {
        TextureDrawer texture;

        Float2 scaling{1.0, 1.0};

        TextureDrawable2D& scaled(float value);

        TextureDrawable2D& scaled(Float2 scaling_);

        void draw(const Vec2& position) const;

        void drawAt(const Vec2& center) const;
    };
}
