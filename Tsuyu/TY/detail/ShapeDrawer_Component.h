#pragma once

#include "ShapeDrawer_Common.h"
#include "TY/IComponent.h"
#include "TY/Shader.h"

namespace TY::ShapeDrawer_detail
{
    const std::string ShaderPath = "asset/shader/shape2d.hlsl";

    inline struct ShapeDrawerComponent* s_component;

    struct ShapeDrawerComponent : IComponent
    {
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

        bool init() override
        {
            assert(not s_component);

            s_component = this;

            return true;
        }

        ~ShapeDrawerComponent()
        {
            if (s_component == this)
            {
                s_component = nullptr;
            }
        }

        void beforeFlush() override
        {
            for (auto it = m_subscribableList.begin(); it != m_subscribableList.end();)
            {
                it->get()->beforeFlush();

                if (it->get()->m_shouldRemove)
                {
                    it = m_subscribableList.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void afterPresent() override
        {
            for (auto it = m_subscribableList.begin(); it != m_subscribableList.end();)
            {
                it->get()->afterPresent();
                ++it;
            }
        }
    };
}
