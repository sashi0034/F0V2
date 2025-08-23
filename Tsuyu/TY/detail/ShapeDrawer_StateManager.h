#pragma once
#include "GraphicsPipelineState.h"
#include "ShapeDrawer_DescriptorManager.h"

namespace TY::ShapeDrawer_detail
{
    class StateManager
    {
    public:
        struct state_type
        {
            GraphicsPipelineStateParams psoParams{};
            DescriptorManager::element_pointer descriptor{};

            static state_type Default(const DescriptorTable& descriptorTable);
        };

        void Reset(const DescriptorTable& descriptorTable)
        {
            m_current = state_type::Default(descriptorTable);
            m_next.reset();
        }

        void RequestDescriptor(
            const DescriptorManager::element_pointer& descriptor,
            const DescriptorTable& descriptorTable)
        {
            assert(descriptor.isValid());

            if (m_current.descriptor != descriptor)
            {
                getNext().descriptor = descriptor;
                getNext().psoParams.descriptorTable = descriptorTable;
            }
        }

        void RequestPixelShader(const PixelShader& ps)
        {
            if (m_current.psoParams.shader.ps.unique_id() != ps.unique_id())
            {
                getNext().psoParams.shader.ps = ps;
            }
        }

        const state_type& Current() const
        {
            return m_current;
        }

        std::optional<state_type> ApplyNext()
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

    private:
        state_type m_current{};

        std::optional<state_type> m_next{};

        state_type& getNext()
        {
            if (m_next.has_value())
            {
                return m_next.value();
            }

            m_next = state_type::Default(m_current.psoParams.descriptorTable);
            m_next->descriptor = m_current.descriptor;
            return m_next.value();
        }
    };
}
