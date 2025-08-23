#include "pch.h"
#include "ShapeDrawer_StateManager.h"

#include "ShapeDrawer_Component.h"

using namespace ShapeDrawer_detail;

namespace
{
    GraphicsPipelineStateParams getDefaultPsoParams(const DescriptorTable& descriptorTable)
    {
        auto&& component = ShapeDrawerComponent::Instance;
        return GraphicsPipelineStateParams{
            .shader = GraphicsShader{component->m_vs, component->m_ps.shape},
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
    SD_StateManager::state_type SD_StateManager::state_type::Default(const DescriptorTable& descriptorTable)
    {
        return state_type{
            .psoParams = getDefaultPsoParams(descriptorTable),
            .descriptor = {}
        };
    }

    void SD_StateManager::Reset(const DescriptorTable& descriptorTable)
    {
        m_current = state_type::Default(descriptorTable);
        m_next.reset();
    }

    void SD_StateManager::RequestDescriptor(
        const SD_DescriptorManager::element_pointer& descriptor,
        const DescriptorTable& descriptorTable)
    {
        assert(descriptor.isValid());

        if (m_current.descriptor != descriptor)
        {
            getNext().descriptor = descriptor;
            getNext().psoParams.descriptorTable = descriptorTable;
        }
    }

    void SD_StateManager::RequestPixelShader(const PixelShader& ps)
    {
        if (m_current.psoParams.shader.ps.unique_id() != ps.unique_id())
        {
            getNext().psoParams.shader.ps = ps;
        }
    }

    std::optional<SD_StateManager::state_type> SD_StateManager::ApplyNext()
    {
        if (m_next.has_value())
        {
            auto previous = std::move(m_current);
            m_current = std::move(m_next.value());
            m_next.reset();
            return previous;
        }

        return std::nullopt;
    }

    SD_StateManager::state_type& SD_StateManager::getNext()
    {
        if (m_next.has_value())
        {
            return m_next.value();
        }

        m_next = state_type::Default(m_current.psoParams.descriptorTable);
        m_next->descriptor = m_current.descriptor;
        return m_next.value();
    }
}
