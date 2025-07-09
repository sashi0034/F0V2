#pragma once

namespace TY
{
    enum class GraphicsCullMode : uint8_t
    {
        None,
        Back,
        Front
    };

    struct GraphicsRasterizerSettings
    {
        GraphicsCullMode cull{GraphicsCullMode::None};

        static GraphicsRasterizerSettings Default3D();
    };

    struct GraphicsDepthSettings
    {
        bool enable{};
        bool writeMask{};

        static GraphicsDepthSettings Default3D();

        GraphicsDepthSettings& setWriteMask(bool writeMask_);
    };
}
