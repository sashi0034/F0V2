#include "pch.h"
#include "ImmediateDrawer_StateManager.h"

#include "ImmediateDrawer_Component.h"

using namespace ImmediateDrawer_detail;

namespace
{
    GraphicsPipelineStateParams getDefaultPsoParams(bool is3D, const DescriptorTable& descriptorTable)
    {
        auto&& component = ImmediateDrawerComponent::Instance;

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
                .descriptorTable = descriptorTable,
                .dynamicDescriptorTable = {
                    DynamicDescriptorEntry{
                        .cbvSlot = 0,
                        .cbvCount = 2,
                    },
                },
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
                .descriptorTable = descriptorTable,
                .dynamicDescriptorTable = {
                    DynamicDescriptorEntry{
                        .cbvSlot = 0,
                        .cbvCount = 2,
                    },
                },
            };
        }
    }
}

namespace TY::ImmediateDrawer_detail
{
    ID_StateManager::state_type ID_StateManager::state_type::Default(
        bool is3D,
        const DescriptorTable& descriptorTable)
    {
        return state_type{
            .psoParams = getDefaultPsoParams(is3D, descriptorTable),
            .descriptor = {},
            .is3D = is3D,
        };
    }

    void ID_StateManager::Reset(const DescriptorTable& descriptorTable)
    {
        m_current = state_type::Default(false, descriptorTable);
        m_next.reset();
    }

    void ID_StateManager::RequestDescriptor(
        const ID_DescriptorManager::element_cursor& descriptor,
        const DescriptorTable& descriptorTable)
    {
        assert(descriptor.isValid());

        if (m_current.descriptor != descriptor)
        {
            fetchNext().descriptor = descriptor;
            fetchNext().psoParams.descriptorTable = descriptorTable;
        }
    }

    void ID_StateManager::RequestPixelShader(const PixelShader& ps)
    {
        if (m_current.psoParams.shader.ps.unique_id() != ps.unique_id())
        {
            fetchNext().psoParams.shader.ps = ps;
        }
    }

    void ID_StateManager::request2D()
    {
        if (m_current.is3D)
        {
            fetchNext().psoParams = getDefaultPsoParams(false, m_current.psoParams.descriptorTable);
            fetchNext().is3D = false;
            // TODO: 2D から設定引き継ぎ?
        }
    }

    void ID_StateManager::request3D()
    {
        if (not m_current.is3D)
        {
            fetchNext().psoParams = getDefaultPsoParams(true, m_current.psoParams.descriptorTable);
            fetchNext().is3D = true;
            // TODO: 3D から設定引き継ぎ?
        }
    }

    std::optional<ID_StateManager::state_type> ID_StateManager::CommitPendingState()
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

    ID_StateManager::state_type& ID_StateManager::fetchNext()
    {
        if (m_next.has_value())
        {
            return m_next.value();
        }

        m_next = m_current;
        return m_next.value();
    }
}
