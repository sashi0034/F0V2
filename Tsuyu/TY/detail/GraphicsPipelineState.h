#pragma once
#include "DescriptorTable.h"
#include "TY/Array.h"
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

    struct PipelineStateParams
    {
        PixelShader pixelShader;
        VertexShader vertexShader;
        std::vector<VertexInputElement> vertexInput;
        bool hasDepth{};
        DescriptorTable descriptorTable{};
    };

    struct PipelineState_impl;

    class GraphicsPipelineState
    {
    public:
        GraphicsPipelineState(const PipelineStateParams& params);

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
