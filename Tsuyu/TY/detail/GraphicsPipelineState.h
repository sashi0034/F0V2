#pragma once
#include "DescriptorTable.h"
#include "DynamicDescriptorTable.h"
#include "TY/Array.h"
#include "TY/GraphicsOptions.h"
#include "TY/Shader.h"
#include "TY/VertexInputElement.h"

namespace TY::detail
{
    struct GraphicsPipelineStateParams
    {
        GraphicsShader shader;

        Array<VertexInputElement> vertexInput;

        GraphicsOptions options;

        DescriptorTable descriptorTable{};

        Array<DynamicDescriptorEntry> dynamicDescriptorTable{};

        bool equalsTo(const GraphicsPipelineStateParams& other) const;
    };

    struct PipelineState_impl;

    class GraphicsPipelineState
    {
    public:
        GraphicsPipelineState() = default;

        GraphicsPipelineState(const GraphicsPipelineStateParams& params);

        DescriptorTable descriptorTable() const;

        void commandSet() const;

        class Internal;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
