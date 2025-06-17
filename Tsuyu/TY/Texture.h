#pragma once
#include "Rect.h"
#include "Shader.h"
#include "TextureSource.h"

namespace TY
{
    struct TextureParams
    {
        TextureSource source; // FIXME? 生から ComPtr はメモリリークするのだろうか
        PixelShader ps;
        VertexShader vs;

        TextureParams& setSource(const TextureSource& source_)
        {
            source = source_;
            return *this;
        }

        TextureParams& setPS(const PixelShader& ps_)
        {
            ps = ps_;
            return *this;
        }

        TextureParams& setVS(const VertexShader& vs_)
        {
            vs = vs_;
            return *this;
        }
    };

    class Texture
    {
        friend struct TextureDrawable2D;

    public:
        Texture() = default;

        Texture(const TextureParams& params);

        Texture(const TextureSource& source, const PixelShader& ps, const VertexShader& vs)
            : Texture(TextureParams{source, ps, vs})
        {
        }

        [[nodiscard]]
        TextureDrawable2D as2D() const;

        void draw3D() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
        std::wstring m_filename;
    };

    struct TextureDrawable2D
    {
        Texture texture;

        Float2 scaling{1.0, 1.0};

        TextureDrawable2D& scaled(float value);

        TextureDrawable2D& scaled(Float2 scaling_);

        void draw(const Vec2& position) const;

        void drawAt(const Vec2& center) const;
    };
}
