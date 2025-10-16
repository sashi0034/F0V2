#pragma once
#include "Array.h"
#include "DXGIFormat.h"

namespace TY
{
    using GraphicsFormat = DXGI_FORMAT;

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

    enum class GraphicsComparisonFunction : uint8_t
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };

    struct GraphicsSamplerOptions
    {
        GraphicsFilterMode filter{GraphicsFilterMode::Linear};
        GraphicsAddressMode addressU{GraphicsAddressMode::Wrap};
        GraphicsAddressMode addressV{GraphicsAddressMode::Wrap};
        GraphicsAddressMode addressW{GraphicsAddressMode::Wrap};
        GraphicsComparisonFunction comparison{GraphicsComparisonFunction::Never};
        int maxAnisotropy{};

        GraphicsSamplerOptions& setAddress(GraphicsAddressMode mode);

        GraphicsSamplerOptions& setFilter(GraphicsFilterMode mode);

        GraphicsSamplerOptions& setComparison(GraphicsComparisonFunction comparison_);

        GraphicsSamplerOptions& setMaxAnisotropy(int maxAnisotropy_);

        bool operator ==(const GraphicsSamplerOptions& other) const = default;
    };

    enum class GraphicsCullMode : uint8_t
    {
        None,
        Back,
        Front
    };

    enum class GraphicsFillMode : uint8_t
    {
        Solid,
        Wireframe
    };

    struct GraphicsRasterizerOptions
    {
        GraphicsCullMode cull{GraphicsCullMode::None};
        GraphicsFillMode fill{GraphicsFillMode::Solid};

        GraphicsRasterizerOptions& setCull(GraphicsCullMode cull_);

        GraphicsRasterizerOptions& setFill(GraphicsFillMode fill_);

        bool operator ==(const GraphicsRasterizerOptions& other) const = default;

        static GraphicsRasterizerOptions Default3D();
    };

    struct GraphicsDepthOptions
    {
        bool enable{};
        bool writeMask{};

        GraphicsDepthOptions& setWriteMask(bool writeMask_);

        bool operator ==(const GraphicsDepthOptions& other) const = default;

        static GraphicsDepthOptions Default3D();
    };

    enum class GraphicsPrimitiveTopology : uint8_t
    {
        TriangleList,
        LineList
    };

    struct GraphicsOptions
    {
        Array<GraphicsSamplerOptions> samplers{GraphicsSamplerOptions()};
        GraphicsRasterizerOptions rasterizer{};
        GraphicsDepthOptions depth{};
        Array<GraphicsFormat> rtvFormats{DXGI_FORMAT_R8G8B8A8_UNORM};
        GraphicsPrimitiveTopology topology{GraphicsPrimitiveTopology::TriangleList};

        GraphicsOptions& setSamplers(const Array<GraphicsSamplerOptions>& samplers_);

        // GraphicsOptions& addSampler(const GraphicsSamplerOptions& samplers_);

        GraphicsOptions& setRasterizer(const GraphicsRasterizerOptions& rasterizer_);

        GraphicsOptions& setDepth(const GraphicsDepthOptions& depth_);

        GraphicsOptions& setRtvFormats(const Array<GraphicsFormat>& formats);

        GraphicsOptions& setTopology(GraphicsPrimitiveTopology topology_);

        bool operator ==(const GraphicsOptions& other) const = default;

        static GraphicsOptions Default3D();
    };
}
