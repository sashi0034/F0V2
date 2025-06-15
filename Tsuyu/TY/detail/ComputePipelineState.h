#pragma once
#include "DescriptorTable.h"
#include "TY/Array.h"
#include "TY/Shader.h"

namespace TY::detail
{
    struct DescriptorTableElement;

    struct ComputePipelineStateParams
    {
        ComputeShader computeShader;
        Array<DescriptorTableElement> descriptorTable;
    };

    class ComputePipelineState
    {
    public:
        ComputePipelineState() = default;

        ComputePipelineState(const ComputePipelineStateParams& params);

        DescriptorTable descriptorTable() const;

        void commandSet() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
