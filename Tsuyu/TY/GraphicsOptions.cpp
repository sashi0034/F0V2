#include "pch.h"
#include "GraphicsOptions.h"

#include "RenderTarget.h"

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

    GraphicsSamplerOptions& GraphicsSamplerOptions::setComparison(GraphicsComparisonFunction comparison_)
    {
        comparison = comparison_;
        return *this;
    }

    GraphicsSamplerOptions& GraphicsSamplerOptions::setMaxAnisotropy(int maxAnisotropy_)
    {
        maxAnisotropy = maxAnisotropy_;
        return *this;
    }

    GraphicsBlendOptions GraphicsBlendOptions::Opaque()
    {
        GraphicsBlendOptions blend;
        blend.blendEnabled = false;
        return blend;
    }

    GraphicsBlendOptions GraphicsBlendOptions::AlphaBlend()
    {
        GraphicsBlendOptions blend;
        blend.blendEnabled = true;
        return blend;
    }

    GraphicsRasterizerOptions& GraphicsRasterizerOptions::setCull(GraphicsCullMode cull_)
    {
        cull = cull_;
        return *this;
    }

    GraphicsRasterizerOptions& GraphicsRasterizerOptions::setFill(GraphicsFillMode fill_)
    {
        fill = fill_;
        return *this;
    }

    GraphicsRasterizerOptions GraphicsRasterizerOptions::Default3D()
    {
        GraphicsRasterizerOptions settings{};
        settings.cull = GraphicsCullMode::Back;
        return settings;
    }

    GraphicsDepthOptions& GraphicsDepthOptions::setTestEnabled(bool testEnabled_)
    {
        testEnabled = testEnabled_;
        return *this;
    }

    GraphicsDepthOptions& GraphicsDepthOptions::setWriteMask(bool writeMask_)
    {
        writeMask = writeMask_;
        return *this;
    }

    GraphicsDepthOptions GraphicsDepthOptions::Default3D()
    {
        GraphicsDepthOptions settings{};
        settings.testEnabled = true;
        settings.writeMask = true;
        return settings;
    }

    GraphicsOptions& GraphicsOptions::setSamplers(const Array<GraphicsSamplerOptions>& samplers_)
    {
        samplers = samplers_;
        return *this;
    }

    GraphicsOptions& GraphicsOptions::setBlend(const GraphicsBlendOptions& blend_)
    {
        blend = blend_;
        return *this;
    }

    // GraphicsOptions& GraphicsOptions::addSampler(const GraphicsSamplerOptions& samplers_)
    // {
    //     samplers.push_back(samplers_);
    //     return *this;
    // }

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

    GraphicsOptions& GraphicsOptions::setRtvFormats(const Array<GraphicsFormat>& formats)
    {
        rtvFormats = formats;
        return *this;
    }

    GraphicsOptions& GraphicsOptions::setTopology(GraphicsPrimitiveTopology topology_)
    {
        topology = topology_;
        return *this;
    }

    GraphicsOptions GraphicsOptions::Default3D()
    {
        GraphicsOptions settings{};
        settings.blend = GraphicsBlendOptions::Opaque();
        settings.rasterizer = GraphicsRasterizerOptions::Default3D();
        settings.depth = GraphicsDepthOptions::Default3D();
        return settings;
    }

    GraphicsOptions GraphicsOptions::FromTarget(const RenderTarget& target)
    {
        return Default3D().setRtvFormats(target.getRtvFormats());
    }
}
