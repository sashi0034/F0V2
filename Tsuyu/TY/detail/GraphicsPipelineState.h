#pragma once
#include "DescriptorTable.h"
#include "TY/Array.h"
#include "TY/GraphicsOptions.h"
#include "TY/Shader.h"

namespace TY::detail
{
    struct VertexInputElement
    {
        std::string semanticName;
        int semanticIndex;
        DXGI_FORMAT format;

        VertexInputElement() = default;

        VertexInputElement(std::string semanticName, int semanticIndex, DXGI_FORMAT format) :
            semanticName(std::move(semanticName)),
            semanticIndex(semanticIndex),
            format(format)
        {
        }
    };

    struct GraphicsPipelineStateParams
    {
        GraphicsShader shader;
        std::vector<VertexInputElement> vertexInput;
        GraphicsOptions options;
        DescriptorTable descriptorTable{};
    };

    struct PipelineState_impl;

    class GraphicsPipelineState
    {
    public:
        GraphicsPipelineState(const GraphicsPipelineStateParams& params);

        DescriptorTable descriptorTable() const;

        void commandSet() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    // class ScopedPipelineState : Uncopyable
    // {
    // public:
    //     explicit ScopedPipelineState(const PipelineState& pipelineState);
    //
    //     ~ScopedPipelineState();
    //
    // private:
    //     size_t m_timestamp;
    // };
}
