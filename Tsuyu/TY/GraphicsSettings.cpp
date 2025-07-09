#include "pch.h"
#include "GraphicsSettings.h"

namespace TY
{
    GraphicsRasterizerSettings GraphicsRasterizerSettings::Default3D()
    {
        GraphicsRasterizerSettings settings{};
        settings.cull = GraphicsCullMode::Back;
        return settings;
    }

    GraphicsDepthSettings GraphicsDepthSettings::Default3D()
    {
        GraphicsDepthSettings settings{};
        settings.enable = true;
        settings.writeMask = true;
        return settings;
    }

    GraphicsDepthSettings& GraphicsDepthSettings::setWriteMask(bool writeMask_)
    {
        writeMask = writeMask_;
        return *this;
    }
}
