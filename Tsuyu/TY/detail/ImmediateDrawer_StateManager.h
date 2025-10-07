#pragma once
#include "GraphicsPipelineState.h"
#include "ImmediateDrawer_DescriptorManager.h"

namespace TY::ImmediateDrawer_detail
{
    class SD_StateManager
    {
    public:
        struct state_type
        {
            GraphicsPipelineStateParams psoParams{};
            SD_DescriptorManager::element_cursor descriptor{};
            bool is3D{}; // 2D if false

            static state_type Default(bool is3D, const DescriptorTable& descriptorTable);
        };

        void Reset(const DescriptorTable& descriptorTable);

        void RequestDescriptor(
            const SD_DescriptorManager::element_cursor& descriptor,
            const DescriptorTable& descriptorTable);

        void RequestPixelShader(const PixelShader& ps);

        void request2D();

        void request3D();

        const state_type& Current() const
        {
            return m_current;
        }

        std::optional<state_type> CommitPendingState();

    private:
        state_type m_current{};

        std::optional<state_type> m_next{};

        state_type& getNext();
    };
}
