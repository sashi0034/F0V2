#pragma once
#include "DescriptorTable.h"
#include "ShaderRegisterStart.h"
#include "TY/Array.h"
#include "TY/Shader.h"

namespace TY::detail
{
    struct DescriptorTableElement;

    struct ComputePipelineStateParams
    {
        ComputeShader computeShader;

        Array<DescriptorTableElement> descriptorTable;

        Array<ShaderRegisterStart> explicitRegisterStarts{};
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
