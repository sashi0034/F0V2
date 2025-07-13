#pragma once
#include "Array.h"

namespace TY
{
    enum class GraphicsAddressMode : uint8_t
    {
        Wrap, // Repeats texture coordinates for tiling.
        Mirror, // Mirrors texture coordinates on each repeat.
        Clamp, // Clamps texture coordinates to edge values.
        Border, // Uses border color for out-of-range coordinates.
        MirrorOnce // Mirrors once then clamps beyond range.
    };

    enum class GraphicsFilterMode : uint8_t
    {
        Nearest,
        Linear,
        Aniso
    };

    struct GraphicsSamplerOptions
    {
        GraphicsAddressMode addressU{GraphicsAddressMode::Wrap};
        GraphicsAddressMode addressV{GraphicsAddressMode::Wrap};
        GraphicsAddressMode addressW{GraphicsAddressMode::Wrap};
        GraphicsFilterMode filter{GraphicsFilterMode::Nearest};

        GraphicsSamplerOptions& setAddress(GraphicsAddressMode mode);

        GraphicsSamplerOptions& setFilter(GraphicsFilterMode mode);
    };

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
        Array<GraphicsSamplerOptions> samplers{GraphicsSamplerOptions()};
        GraphicsRasterizerOptions rasterizer{};
        GraphicsDepthOptions depth{};

        GraphicsOptions& setSamplers(const Array<GraphicsSamplerOptions>& samplers_);

        GraphicsOptions& setRasterizer(const GraphicsRasterizerOptions& rasterizer_);

        GraphicsOptions& setDepth(const GraphicsDepthOptions& depth_);

        static GraphicsOptions Default3D();
    };
}
