#include "pch.h"
#include "Demo_FSR2_ST.h"

#include "imgui/imgui.h"

#include <dxgi1_6.h>

#include "TY/ConstantBufferWrapper.h"
#include "TY/Gamepad.h"
#include "TY/Graphics3D.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"

#include "TY/Shader.h"
#include "TY/System.h"

#include "TY/Math.h"
#include "TY/ModelDrawer.h"
#include "TY/ModelLoader.h"
#include "TY/Mouse.h"
#include "TY/RenderTarget.h"
#include "TY/Scene.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"

#include "ffx_fsr2.h"
#pragma comment(lib, "ffx_fsr2_api_x64.lib")
#pragma comment(lib, "ffx_fsr2_api_dx12_x64.lib")

#include "dx12/ffx_fsr2_dx12.h"
#include "TY/Logger.h"
#include "TY/detail/EngineRenderContext.h"

using namespace TY;

namespace
{
    struct Shadertoy_b10
    {
        Float2 g_screenResolution{};
        Float2 g_mousePosition{};
        Float2 g_jitterOffset{};
    };

    struct ToyModelBuffer : IGenericModelBuffer
    {
        GenericModelShapeBufferElement m_shape{};

        ToyModelBuffer()
        {
            m_shape.materialIndex = 0;
            m_shape.indexBuffer = IndexBuffer::Placeholder(6);
        }

        int shapeCount() const override
        {
            return 1; // Assuming a single shape
        }

        GenericModelShapeBufferElement shapeAt(int index) const override
        {
            return m_shape;
        }

        int materialCount() const override
        {
            return 1; // Assuming a single material for the shape
        }

        ConstantBufferCore materialCbv() const override
        {
            return ConstantBufferCore{1};
        }

        Array<Array<ShaderResourceType>> materialSrv() const override
        {
            return {};
        }
    };

    constexpr float fovFarZ = 1000.0f;

    constexpr float fovNearZ = 0.1f;
}

struct Demo_FSR2_ST_impl
{
    struct
    {
        GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

        GraphicsShader shadertoy{GraphicsShader::VS_PS("asset/shader/shadertoy.hlsl")};
    } m_shaders;

    Mat4x4 m_projectionMat{};

    GenericModelDrawer m_toyDrawer{};

    ConstantBufferWrapper<Shadertoy_b10> m_toyCB{};

    Demo_FSR2_ST_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        m_toyDrawer = GenericModelDrawerParams{}
                      .setModel(std::make_unique<ToyModelBuffer>())
                      .setVertexInput({})
                      .setShader(m_shaders.shadertoy)
                      .setOptions(GraphicsOptions{})
                      .setCbv10AndLater({m_toyCB});

        InitFsr2();
    }

    // -----------------------------------------------

    FfxFsr2ContextDescription m_initializationParameters = {};

    FfxFsr2Context m_context;

    ComPtr<ID3D12Resource> m_motionVectorTex;

    ComPtr<ID3D12Resource> m_upscaledOutputTex;

    TextureDrawer m_upscaledOutputDrawer{};

    RenderTarget m_inputRT{{.size = Scene::Size() * 0.5, .clearColor = ColorF32{0.0f, 1.0f}}};

    TextureDrawer m_debugInputRtDrawer{};

    Float2 m_jitterOffset{};

    void InitFsr2()
    {
        const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeDX12();
        void* scratchBuffer = malloc(scratchBufferSize);

        auto device = detail::EngineRenderContext::GetDevice();
        const FfxErrorCode errorCode = ffxFsr2GetInterfaceDX12(
            &m_initializationParameters.callbacks, device, scratchBuffer, scratchBufferSize);

        m_initializationParameters.device = ffxGetDeviceDX12(device);
        m_initializationParameters.maxRenderSize.width = Scene::Size().x * 0.5;
        m_initializationParameters.maxRenderSize.height = Scene::Size().y * 0.5;
        m_initializationParameters.displaySize.width = Scene::Size().x;
        m_initializationParameters.displaySize.height = Scene::Size().y;
        m_initializationParameters.flags = {};
        m_initializationParameters.flags |= FFX_FSR2_ENABLE_AUTO_EXPOSURE;
        m_initializationParameters.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
        m_initializationParameters.fpMessage = onFSR2Msg;

        const uint64_t memoryUsageBefore = getMemoryUsageSnapshot(device);
        ffxFsr2ContextCreate(&m_context, &m_initializationParameters);
        const uint64_t memoryUsageAfter = getMemoryUsageSnapshot(device);
        auto memoryUsageInMegabytes = (memoryUsageAfter - memoryUsageBefore) * 1e-6f;
        LogInfo(std::format("FSR2 memory usage: {:.2} MB\n", memoryUsageInMegabytes));

        createMotionVectors();

        createUpscaledOutput();

        m_upscaledOutputDrawer = TextureDrawer{
            TextureDrawerParams{}
            .setShader(m_shaders.default2d)
            .setTexture(TextureResource{m_upscaledOutputTex.Get()})
        };

        m_debugInputRtDrawer = TextureDrawer{
            TextureDrawerParams{}
            .setShader(m_shaders.default2d)
            .setTexture({m_inputRT.asShaderResource()})
        };
    }

    void createMotionVectors()
    {
        UINT width = Scene::Size().x * 0.5; // レンダー解像度
        UINT height = Scene::Size().y * 0.5;

        // リソースディスクリプション
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R16G16_FLOAT; // float2 motion vectors
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        // デフォルトヒープに確保
        auto device = detail::EngineRenderContext::GetDevice();
        auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COMMON, // 初期状態
            nullptr,
            IID_PPV_ARGS(&m_motionVectorTex));

        // TODO: destoy
    }

    void createUpscaledOutput()
    {
        UINT width = Scene::Size().x;
        UINT height = Scene::Size().y;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // UAV OK

        auto device = detail::EngineRenderContext::GetDevice();
        auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_upscaledOutputTex));
    }

    void DrawFsr2()
    {
        FfxFsr2DispatchDescription dispatchParameters = {};
        dispatchParameters.commandList =
            ffxGetCommandListDX12(detail::EngineRenderContext::GetCommandList(detail::CommandListType::Draw));
        dispatchParameters.color =
            ffxGetResourceDX12(&m_context, m_inputRT.getRtvResource(0), L"FSR2_InputColor");
        dispatchParameters.depth = ffxGetResourceDX12(&m_context, m_inputRT.getDsvResource(), L"FSR2_InputDepth");
        dispatchParameters.motionVectors = ffxGetResourceDX12(&m_context, m_motionVectorTex.Get(),
                                                              L"FSR2_InputMotionVectors");
        dispatchParameters.exposure = ffxGetResourceDX12(&m_context, nullptr, L"FSR2_InputExposure");

        static float s_sharpness = 0.1f;
        static bool s_jitterEnabled{};

        dispatchParameters.output = ffxGetResourceDX12(
            &m_context,
            m_upscaledOutputTex.Get(),
            L"FSR2_OutputUpscaledColor",
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        dispatchParameters.jitterOffset.x = m_jitterOffset.x;
        dispatchParameters.jitterOffset.y = m_jitterOffset.y;

        if (s_jitterEnabled)
        {
            const int renderW = m_initializationParameters.maxRenderSize.width; // = m_inputRT.size().x
            const int displayW = m_initializationParameters.displaySize.width; // = Scene::Size().x

            const int jitterPhaseCount = ffxFsr2GetJitterPhaseCount(renderW, displayW);

            ffxFsr2GetJitterOffset(&m_jitterOffset.x, &m_jitterOffset.y, System::FrameCount(), jitterPhaseCount);

            // std::cout << "jitter offset: " << m_jitterOffset.x << ", " << m_jitterOffset.y << "\n";
        }

        dispatchParameters.motionVectorScale.x = static_cast<float>(Scene::Size().x) * 0.5f;
        dispatchParameters.motionVectorScale.y = static_cast<float>(Scene::Size().y) * 0.5f;
        dispatchParameters.reset = false;
        dispatchParameters.enableSharpening = true;
        dispatchParameters.sharpness = s_sharpness;
        dispatchParameters.frameTimeDelta = System::DeltaTime() * 1000.0f;
        dispatchParameters.preExposure = 1.0f;
        dispatchParameters.renderSize.width = Scene::Size().x * 0.5;
        dispatchParameters.renderSize.height = Scene::Size().y * 0.5;
        dispatchParameters.cameraFar = fovFarZ;
        dispatchParameters.cameraNear = fovNearZ;
        dispatchParameters.cameraFovAngleVertical = Math::ToRadians(75.0f);

        FfxErrorCode errorCode = ffxFsr2ContextDispatch(&m_context, &dispatchParameters);
        FFX_ASSERT(errorCode == FFX_OK);

        m_upscaledOutputDrawer.as2D().resized(Scene::Size()).draw(Float2{});

        ImGui::Begin("FSR2");

        ImGui::Checkbox("Jitter", &s_jitterEnabled);

        ImGui::SliderFloat("Sharpness", &s_sharpness, 0.0f, 1.0f);

        ImGui::End();
    }

    static void onFSR2Msg(FfxFsr2MsgType type, const wchar_t* message)
    {
        if (type == FFX_FSR2_MESSAGE_TYPE_ERROR)
        {
            LogInfo(L"FSR2_API_DEBUG_ERROR: ");
        }
        else if (type == FFX_FSR2_MESSAGE_TYPE_WARNING)
        {
            LogInfo(L"FSR2_API_DEBUG_WARNING: ");
        }

        LogInfo(message);
        LogInfo(L"\n");
    }

    static bool isLuidsEqual(LUID luid1, LUID luid2)
    {
        return memcmp(&luid1, &luid2, sizeof(LUID)) == 0;
    }

    static uint64_t getMemoryUsageSnapshot(ID3D12Device* device)
    {
        uint64_t memoryUsage = -1;
        IDXGIFactory* pFactory = nullptr;
        if (SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory))))
        {
            IDXGIAdapter* pAdapter = nullptr;
            UINT i = 0;
            while (pFactory->EnumAdapters(i++, &pAdapter) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_ADAPTER_DESC desc{};
                if (SUCCEEDED(pAdapter->GetDesc(&desc)))
                {
                    if (isLuidsEqual(desc.AdapterLuid, device->GetAdapterLuid()))
                    {
                        IDXGIAdapter4* pAdapter4 = nullptr;

                        if (SUCCEEDED(pAdapter->QueryInterface(IID_PPV_ARGS(&pAdapter4))))
                        {
                            DXGI_QUERY_VIDEO_MEMORY_INFO info{};
                            if (SUCCEEDED(pAdapter4->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
                            {
                                memoryUsage = info.CurrentUsage;
                            }

                            pAdapter4->Release();
                        }
                    }

                    pAdapter->Release();
                }
            }

            pFactory->Release();
        }

        return memoryUsage;
    }

    // -----------------------------------------------

    void draw3D()
    {
        m_toyCB->g_screenResolution = Float2{m_inputRT.size()};
        m_toyCB->g_mousePosition = Mouse::PosF() * 0.5f;
        m_toyCB->g_jitterOffset = m_jitterOffset;
        m_toyCB.upload();

        m_toyDrawer.draw();
    }

    void Update()
    {
        static bool s_fsrEnabled{true};

        {
            auto bind = m_inputRT.scopedBind();
            draw3D();
        }

        if (s_fsrEnabled)
        {
            DrawFsr2();
        }
        else
        {
            m_debugInputRtDrawer.as2D().resized(Scene::Size()).draw(Float2{});
        }

        {
            ImGui::Begin("System Settings");

            static bool s_sleep{};;
            ImGui::Checkbox("Sleep", &s_sleep);

            if (s_sleep)
            {
                System::Sleep(500);
            }

            ImGui::Checkbox("FSR2 Enabled", &s_fsrEnabled);

            ImGui::End();
        }
    }

private:
};

void Demo_FSR2_ST()
{
    Scene::RequestResize(Size{1920, 1080} * 2);
    System::Update();

    Demo_FSR2_ST_impl impl{};

    while (System::Update())
    {
        impl.Update();
    }
}
