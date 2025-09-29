#include "pch.h"
#include "ShapeDrawer_StateManager.h"

#include "ShapeDrawer_Component.h"

using namespace ShapeDrawer_detail;

namespace
{
    GraphicsPipelineStateParams getDefaultPsoParams(bool is3D, const DescriptorTable& descriptorTable)
    {
        auto&& component = ShapeDrawerComponent::Instance;

        if (not is3D)
        {
            return GraphicsPipelineStateParams{
                .shader = GraphicsShader{component->m_vs2d, component->m_ps2d.shape},
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT},
                    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT},
                },
                .options = GraphicsOptions()
                           .setSamplers({
                               GraphicsSamplerOptions().setFilter(GraphicsFilterMode::Linear)
                           })
                           .setTopology(GraphicsPrimitiveTopology::TriangleList),
                // .setRasterizer(GraphicsRasterizerOptions().setFill(GraphicsFillMode::Wireframe)),
                .descriptorTable = descriptorTable
            };
        }
        else
        {
            return GraphicsPipelineStateParams{
                .shader = GraphicsShader{component->m_vs3d, component->m_ps3d.shape},
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT},
                },
                .options = GraphicsOptions::Default3D()
                           .setSamplers({
                               GraphicsSamplerOptions().setFilter(GraphicsFilterMode::Linear)
                           })
                           .setTopology(GraphicsPrimitiveTopology::LineList),
                .descriptorTable = descriptorTable
            };
        }
    }
}

namespace TY::ShapeDrawer_detail
{
    SD_StateManager::state_type SD_StateManager::state_type::Default(
        bool is3D,
        const DescriptorTable& descriptorTable)
    {
        return state_type{
            .psoParams = getDefaultPsoParams(is3D, descriptorTable),
            .descriptor = {}
        };
    }

    void SD_StateManager::Reset(const DescriptorTable& descriptorTable)
    {
        m_current = state_type::Default(false, descriptorTable);
        m_next.reset();
    }

    void SD_StateManager::RequestDescriptor(
        const SD_DescriptorManager::element_cursor& descriptor,
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

    void SD_StateManager::request2D()
    {
        if (m_current.is3D)
        {
            getNext().psoParams = getDefaultPsoParams(false, m_current.psoParams.descriptorTable);
            // TODO: 2D から設定引き継ぎ?
        }
    }

    void SD_StateManager::request3D()
    {
        if (not m_current.is3D)
        {
            getNext().psoParams = getDefaultPsoParams(true, m_current.psoParams.descriptorTable);
            // TODO: 3D から設定引き継ぎ?
        }
    }

    std::optional<SD_StateManager::state_type> SD_StateManager::CommitPendingState()
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

        m_next = state_type::Default(false, m_current.psoParams.descriptorTable);
        m_next->descriptor = m_current.descriptor;
        return m_next.value();
    }
}
