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

        void Reset(const DescriptorTable& descriptorTable);

        void RequestDescriptor(
            const DescriptorManager::element_pointer& descriptor,
            const DescriptorTable& descriptorTable);

        void RequestPixelShader(const PixelShader& ps);

        const state_type& Current() const
        {
            return m_current;
        }

        std::optional<state_type> ApplyNext();

    private:
        state_type m_current{};

        std::optional<state_type> m_next{};

        state_type& getNext();
    };
}
