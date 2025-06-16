#include "pch.h"
#include "Demo_Gpgpu.h"

#include "LivePPAddon.h"
#include "TY/Gpgpu.h"
#include "TY/Logger.h"
#include "TY/System.h"

using namespace TY;

namespace
{
}

struct Demo_Gpgpu_Impl
{
    ComputeShader m_computeShader{};
    GpgpuBuffer<uint32_t> m_buffer{};
    GpgpuBuffer<uint32_t> m_readonlyData{};
    Gpgpu m_gpgpu{};

    Demo_Gpgpu_Impl()
    {
        m_computeShader = ComputeShader{ShaderParams::CS("asset/shader/simple_compute.hlsl")};

        m_buffer = GpgpuBuffer<uint32_t>::Writable(100);
        m_readonlyData = GpgpuBuffer<uint32_t>::Readonly(50);

        for (int i = 0; i < m_readonlyData.data().size(); ++i)
        {
            m_readonlyData.data()[i] = i;
        }

        m_gpgpu = Gpgpu{
            GpgpuParams{}
            .setCS(m_computeShader)
            .setBuffers({m_buffer, m_readonlyData})
        };

        m_gpgpu.compute();
    }

    void Update()
    {
        {
            ImGui::Begin("Compute Shader");

            const auto& data = m_buffer.data();
            ImGui::Text("Element Count: %d", data.size());

            ImGui::BeginGroup();
            for (size_t i = 0; i < data.size(); ++i)
            {
                ImGui::Text("[%d] = %u", i, data[i]);
                if (i % 4 == 3) ImGui::NewLine();
            }

            ImGui::EndGroup();

            if (ImGui::Button("Compute"))
            {
                m_gpgpu.compute();

                LogInfo.writeln("Computed!");
            }

            ImGui::End();
        }
    }
};

void Demo_Gpgpu()
{
    Demo_Gpgpu_Impl impl{};

    while (System::Update())
    {
#ifdef _DEBUG
        Util::AdvanceLivePP();
#endif

        impl.Update();
    }
}
