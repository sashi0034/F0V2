#pragma once
#include "Rect.h"
#include "Shader.h"
#include "ShaderResourceTexture.h"
#include "TextureSource.h"

namespace TY
{
    struct TextureDrawerParams
    {
        ShaderResourceTexture texture;
        PixelShader ps;
        VertexShader vs;
        bool hasDepth{false};

        TextureDrawerParams& loadTexture(const TextureSource& source);

        TextureDrawerParams& setTexture(const ShaderResourceTexture& texture_);

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

        TextureDrawerParams& setShaders(const PixelShader& ps_, const VertexShader& vs_)
        {
            ps = ps_;
            vs = vs_;
            return *this;
        }

        TextureDrawerParams& setShaders(const GraphicsShader& shader)
        {
            ps = shader.ps;
            vs = shader.vs;
            return *this;
        }

        TextureDrawerParams& setHasDepth(bool hasDepth_)
        {
            hasDepth = hasDepth_;
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

        TextureDrawable2D& resized(Float2 size);

        void draw(const Vec2& position) const;

        void drawAt(const Vec2& center) const;
    };
}
