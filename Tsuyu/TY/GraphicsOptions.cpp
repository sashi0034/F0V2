#include "pch.h"
#include "GraphicsOptions.h"

namespace TY
{
    GraphicsSamplerOptions& GraphicsSamplerOptions::setAddress(GraphicsAddressMode mode)
    {
        addressU = mode;
        addressV = mode;
        addressW = mode;
        return *this;
    }

    GraphicsSamplerOptions& GraphicsSamplerOptions::setFilter(GraphicsFilterMode mode)
    {
        filter = mode;
        return *this;
    }

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

    GraphicsOptions& GraphicsOptions::setSamplers(const Array<GraphicsSamplerOptions>& samplers_)
    {
        samplers = samplers_;
        return *this;
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
