#pragma once
#include "Rect.h"
#include "Shader.h"
#include "TextureHandle.h"

namespace TY
{
    struct TextureDrawerParams
    {
        TextureHandle texture;

        GraphicsShader shader;

        bool hasDepth{false};

        TextureDrawerParams& setTexture(const TextureHandle& texture_);

        TextureDrawerParams& setPS(const PixelShader& ps_)
        {
            shader.ps = ps_;
            return *this;
        }

        TextureDrawerParams& setVS(const VertexShader& vs_)
        {
            shader.vs = vs_;
            return *this;
        }

        TextureDrawerParams& setShader(const PixelShader& ps_, const VertexShader& vs_)
        {
            shader.ps = ps_;
            shader.vs = vs_;
            return *this;
        }

        TextureDrawerParams& setShader(const GraphicsShader& shader_)
        {
            shader.ps = shader_.ps;
            shader.vs = shader_.vs;
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
