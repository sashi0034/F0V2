#include "pch.h"
#include "GraphicsOptions.h"

namespace TY
{
    GraphicsRasterizerOptions& GraphicsRasterizerOptions::setCull(GraphicsCullMode cull_)
    {
        cull = cull_;
        return *this;
    }

    GraphicsRasterizerOptions GraphicsRasterizerOptions::Default3D()
    {
        GraphicsRasterizerOptions settings{};
        settings.cull = GraphicsCullMode::Back;
        return settings;
    }

    GraphicsDepthOptions& GraphicsDepthOptions::setWriteMask(bool writeMask_)
    {
        writeMask = writeMask_;
        return *this;
    }

    GraphicsDepthOptions GraphicsDepthOptions::Default3D()
    {
        GraphicsDepthOptions settings{};
        settings.enable = true;
        settings.writeMask = true;
        return settings;
    }

    GraphicsOptions& GraphicsOptions::setRasterizer(const GraphicsRasterizerOptions& rasterizer_)
    {
        rasterizer = rasterizer_;
        return *this;
    }

    GraphicsOptions& GraphicsOptions::setDepth(const GraphicsDepthOptions& depth_)
    {
        depth = depth_;
        return *this;
    }

    GraphicsOptions GraphicsOptions::Default3D()
    {
        GraphicsOptions settings{};
        settings.rasterizer = GraphicsRasterizerOptions::Default3D();
        settings.depth = GraphicsDepthOptions::Default3D();
        return settings;
    }
}
