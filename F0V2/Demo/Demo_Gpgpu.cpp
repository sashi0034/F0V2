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

    Gpgpu<uint32_t> m_gpgpu{};

    Demo_Gpgpu_Impl()
    {
        m_computeShader = ComputeShader{ShaderParams::CS("asset/shader/simple_compute.hlsl")};

        m_gpgpu = Gpgpu<uint32_t>{
            {
                .cs = m_computeShader,
                .elementCount = 64
            }
        };

        m_gpgpu.compute();
    }

    void Update()
    {
        {
            ImGui::Begin("Compute Shader");

            const auto& data = m_gpgpu.data();
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
