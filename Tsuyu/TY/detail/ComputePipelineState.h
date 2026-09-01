#pragma once
#include "CommandListManager.h"
#include "DescriptorEntry.h"
#include "DynamicDescriptorEntry.h"
#include "TY/Array.h"
#include "TY/GraphicsOptions.h"
#include "TY/Shader.h"

namespace TY::detail
{
    struct DescriptorEntry;

    struct ComputePipelineStateParams
    {
        ComputeShader computeShader;

        Array<GraphicsSamplerOptions> samplers{GraphicsSamplerOptions()};

        Array<DescriptorEntry> descriptorTable;

        Array<DynamicDescriptorEntry> dynamicDescriptorTable{};
    };

    class ComputePipelineState
    {
    public:
        ComputePipelineState() = default;

        ComputePipelineState(const ComputePipelineStateParams& params);

        DescriptorTable descriptorTable() const;

        void commandSet(CommandListType commandList) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
