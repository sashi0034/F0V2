#pragma once

#include "TY/IComponent.h"
#include "TY/Shader.h"

namespace TY::ImmediateDrawer_detail
{
    const std::string ShaderPath2D = "engine/shape2d.hlsl";
    const std::string ShaderPath3D = "engine/shape3d.hlsl";

    struct ImmediateDrawerComponent : IComponent
    {
        static inline ImmediateDrawerComponent* Instance{};

        VertexShader m_vs2d{ShaderPath2D, "VS"};

        struct
        {
            PixelShader shape{ShaderPath2D, "PS_Shape"};

            PixelShader squareDot{ShaderPath2D, "PS_SquareDot"};

            PixelShader roundDot{ShaderPath2D, "PS_RoundDot"};

            PixelShader bitmapFont{ShaderPath2D, "PS_BitmapFont"};

            PixelShader sdfFont{ShaderPath2D, "PS_SdfFont"};
        } m_ps2d{};

        VertexShader m_vs3d{ShaderPath3D, "VS"};

        struct
        {
            PixelShader shape{ShaderPath3D, "PS_Shape"};
        } m_ps3d{};

        bool init() override;

        ~ImmediateDrawerComponent() override;
    };
}
