#include "pch.h"
#include "ShapeDrawer_StateManager.h"

#include "ShapeDrawer_Component.h"

using namespace ShapeDrawer_detail;

namespace
{
    GraphicsPipelineStateParams getDefaultPsoParams(const DescriptorTable& descriptorTable)
    {
        return GraphicsPipelineStateParams{
            .shader = GraphicsShader{s_component->m_vs, s_component->m_ps.shape},
            .vertexInput = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT},
                {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT},
            },
            .options = GraphicsOptions(),
            // .setRasterizer(GraphicsRasterizerOptions().setFill(GraphicsFillMode::Wireframe)),
            .descriptorTable = descriptorTable
        };
    }
}

namespace TY::ShapeDrawer_detail
{
    StateManager::state_type StateManager::state_type::Default(const DescriptorTable& descriptorTable)
    {
        return state_type{
            .psoParams = getDefaultPsoParams(descriptorTable),
            .descriptor = {}
        };
    }
}
