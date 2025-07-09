#pragma once

namespace TY
{
    enum class GraphicsCullMode : uint8_t
    {
        None,
        Back,
        Front
    };

    struct GraphicsRasterizerOptions
    {
        GraphicsCullMode cull{GraphicsCullMode::None};

        GraphicsRasterizerOptions& setCull(GraphicsCullMode cull_);

        static GraphicsRasterizerOptions Default3D();
    };

    struct GraphicsDepthOptions
    {
        bool enable{};
        bool writeMask{};

        GraphicsDepthOptions& setWriteMask(bool writeMask_);

        static GraphicsDepthOptions Default3D();
    };

    struct GraphicsOptions
    {
        GraphicsRasterizerOptions rasterizer{};
        GraphicsDepthOptions depth{};

        GraphicsOptions& setRasterizer(const GraphicsRasterizerOptions& rasterizer_);

        GraphicsOptions& setDepth(const GraphicsDepthOptions& depth_);

        static GraphicsOptions Default3D();
    };
}
