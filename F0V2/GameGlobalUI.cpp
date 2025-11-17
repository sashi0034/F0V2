#include "pch.h"
#include "GameGlobalUI.h"

#include "GamePalette.h"
#include "GM/DebugService.h"
#include "TY/Array.h"
#include "TY/GpuMetrics.h"
#include "TY/KeyboardInput.h"
#include "TY/Mouse.h"
#include "TY/System.h"
#include "Util/ImmediatePrint.h"

namespace
{
    void debugUI()
    {
#if defined(_DEBUG)
        ImGui::Begin("System Window");

        static bool s_sleep{};;
        if (ImGui::Checkbox("Sleep", &s_sleep); s_sleep)
        {
            System::Sleep(500);
        }

        ImGui::PushStyleColor(ImGuiCol_Button, GamePalette::DarkOrange.toFloat4().cast<ImVec4>());
        {
            if (ImGui::Button("Toggle Editor"))
            {
                g_debugService.editorEnabled = not g_debugService.editorEnabled;
            }
        }
        ImGui::PopStyleColor();

        ImGui::Text("GPU Memory Usage: %.2f MB", GpuMetrics::MemoryUsage().estimateLocalUsageInMB());

        {
            static Array<float> s_resentBuffer{};
            static float s_measuredTime{};
            s_resentBuffer.push_back(GpuMetrics::LastExecutionMilliseconds());
            if (s_resentBuffer.size() > 30)
            {
                s_measuredTime =
                    std::accumulate(s_resentBuffer.begin(), s_resentBuffer.end(), 0.0f) / s_resentBuffer.size();
                s_resentBuffer.clear();
            }

            ImGui::Text("GPU Execution Time:");

            ImGui::BulletText(std::format("[1]:    {:.02f} ms", GpuMetrics::LastExecutionMilliseconds()).c_str());

            ImGui::BulletText(std::format("[1:30]: {:.02f} ms", s_measuredTime).c_str());
        }

        ImGui::Text("Mouse Position: (%.2f, %.2f)", Mouse::PosF().x, Mouse::PosF().y);

        ImGui::SeparatorText("g_debugService");

        ImGui::Checkbox("editorEnabled", &g_debugService.editorEnabled);

        ImGui::SliderFloat("cameraSpeed", &g_debugService.cameraSpeed, 1.0f, 10.0f);

        ImGui::InputInt("monitorMachineId", &g_debugService.monitorMachineId);

        ImGui::Checkbox("diablePlayerInput", &g_debugService.disablePlayerInput);

        ImGui::End();
#endif
    }

    bool s_statsVisible{true};

    void printStats()
    {
        if (not s_statsVisible)
        {
            return;
        }

        static float s_cpuTime{};
        static float s_gpuTime{};
        static float s_accumulatedCpuTime{};
        static float s_accumulatedGpuTime{};
        static int s_count{};

        s_count++;
        s_accumulatedCpuTime += System::DeltaTime();
        s_accumulatedGpuTime += GpuMetrics::LastExecutionMilliseconds();
        if (s_count >= 30)
        {
            s_cpuTime = s_accumulatedCpuTime / s_count;
            s_gpuTime = s_accumulatedGpuTime / s_count;
            s_accumulatedCpuTime = 0.0f;
            s_accumulatedGpuTime = 0.0f;
            s_count = 0;
        }

        const int fps = (s_cpuTime > 0.0f) ? static_cast<int>(1.0f / s_cpuTime) : 0;
        ImmediatePrint_BottomLeft("CPU: {:.02f} ms/frame ({} FPS)", s_cpuTime * 1000.0f, fps);
        ImmediatePrint_BottomLeft("GPU: {:.02f} ms/frame", s_gpuTime);
        ImmediatePrint_BottomLeft("GPU Memory: {:.02f} MB", GpuMetrics::MemoryUsage().estimateLocalUsageInMB());
    }
}

void F0V2::DrawGameGlobalUI()
{
    if (KeyF10.down())
    {
        s_statsVisible = not s_statsVisible;
    }

#if defined(_DEBUG)
    debugUI();
#else
    printStats();
#endif
}

bool F0V2::IsGameStatsVisible()
{
    return s_statsVisible;
}
