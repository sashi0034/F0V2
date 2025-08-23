#pragma once

#include "ShapeDrawer_Common.h"
#include "TY/IComponent.h"
#include "TY/Shader.h"

namespace TY::ShapeDrawer_detail
{
    const std::string ShaderPath = "asset/shader/shape2d.hlsl";

    struct ShapeDrawerComponent : IComponent
    {
        static inline ShapeDrawerComponent* Instance{};

        struct Subscribable
        {
            virtual ~Subscribable() = default;

            bool m_shouldRemove{};

            virtual void beforeFlush() = 0;

            virtual void afterPresent() = 0;
        };

        VertexShader m_vs{ShaderPath, "VS"};

        struct
        {
            PixelShader shape{ShaderPath, "PS_Shape"};

            PixelShader squareDot{ShaderPath, "PS_SquareDot"};

            PixelShader roundDot{ShaderPath, "PS_RoundDot"};

            PixelShader bitmapFont{ShaderPath, "PS_BitmapFont"};
        } m_ps{};

        Array<std::shared_ptr<Subscribable>> m_subscribableList{};

        bool init() override;

        ~ShapeDrawerComponent() override;

        void beforeFlush() override;

        void afterPresent() override;
    };
}
