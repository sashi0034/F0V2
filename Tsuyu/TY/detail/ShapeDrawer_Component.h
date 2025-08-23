#pragma once

#include "TY/IComponent.h"
#include "TY/Shader.h"

namespace TY::ShapeDrawer_detail
{
    const std::string ShaderPath = "engine/shape2d.hlsl";

    struct ShapeDrawerComponent : IComponent
    {
        static inline ShapeDrawerComponent* Instance{};

        VertexShader m_vs{ShaderPath, "VS"};

        struct
        {
            PixelShader shape{ShaderPath, "PS_Shape"};

            PixelShader squareDot{ShaderPath, "PS_SquareDot"};

            PixelShader roundDot{ShaderPath, "PS_RoundDot"};

            PixelShader bitmapFont{ShaderPath, "PS_BitmapFont"};

            PixelShader sdfFont{ShaderPath, "PS_SdfFont"};
        } m_ps{};

        bool init() override;

        ~ShapeDrawerComponent() override;
    };
}
