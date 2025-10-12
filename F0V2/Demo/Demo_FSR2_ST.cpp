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
    struct LambertLight_b4
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
    };

    struct PhongLight_b4
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
        alignas(16) Float3 eyePosition{};
        alignas(16) Float3 ambientColor{};
    };

    struct Skydome_b4
    {
        alignas(16) ColorF32 topColor;
        alignas(16) ColorF32 bottomColor;
        float sphereRadius{};
    };

    struct Pose
    {
        Float3 position{};
        Quaternion rotation{}; // Euler angles in radians

        Mat4x4 getMatrix() const
        {
            return Mat4x4::Identity()
                   .rotated(rotation)
                   .translated(position);
        }

        Float3 eulerAngles() const
        {
            return rotation.eulerAngles();
        }
    };

    TextureResource makeGroundPlane(
        const Size& size, int lineSpacing, const UnifiedColor& lineColor, const UnifiedColor& backColor)
    {
        Image image{size, backColor};
        const ColorU8 backColor2 = backColor.toColorU8().multiplied(0.9f);

        for (int x = 0; x < size.x; x += 2)
        {
            for (int y = 0; y < size.y; y += 2)
            {
                image[Point{x, y}] = backColor2;
            }
        }

        const Size padding = (size % lineSpacing) / 2;

        for (int x = padding.x; x < size.x; x += lineSpacing)
        {
            for (int y = 0; y < size.y; y++)
            {
                image[Point{x, y}] = lineColor;
            }
        }

        for (int y = padding.y; y < size.y; y += lineSpacing)
        {
            for (int x = 0; x < size.x; x++)
            {
                image[Point{x, y}] = lineColor;
            }
        }

        return TextureResource{image};
    }

    constexpr float groundPositionY = -10.0f;

    constexpr float fovFarZ = 1000.0f;

    constexpr float fovNearZ = 0.1f;
}

struct Demo_FSR2_ST_impl
{
    struct
    {
        GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

        GraphicsShader model{GraphicsShader::VS_PS("asset/shader/model.hlsl")};

        // GraphicsShader lambert{GraphicsShader::VS_PS("asset/shader/lambert.hlsl")};

        GraphicsShader phong{GraphicsShader::VS_PS("asset/shader/phong.hlsl")};

        GraphicsShader skydome{GraphicsShader::VS_PS("asset/shader/skydome.hlsl")};
    } m_shaders;

    struct
    {
        ModelBuffer playerModel{ModelLoader::Load("asset/model/tie_fighter.obj")};

        ModelBuffer mountainModel{ModelLoader::Load("asset/model/dirty_plane.obj")};
    } m_models;

    struct
    {
        ConstantBufferWrapper<PhongLight_b4> phongLight{};
    } m_cb;

    SimpleCamera3D m_camera{};

    ModelDrawer m_skydomeModel{};

    Mat4x4 m_projectionMat{};

    ConstantBufferWrapper<LambertLight_b4> m_planeLight{};

    ModelDrawer m_groundPlaneDrawer{};

    ModelDrawer m_playerDrawer{};
    Pose m_playerPose{};

    ModelDrawer m_mountainDrawer{};

    Demo_FSR2_ST_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        resetCamera();

        auto skydome_b4 = ConstantBufferWrapper<Skydome_b4>{};
        skydome_b4->topColor = ColorF32{0.3f, 0.0f, 1.0f};
        skydome_b4->bottomColor = ColorF32{1.0f, 1.0f, 1.0f};
        skydome_b4->sphereRadius = fovFarZ;
        skydome_b4.upload();

        m_skydomeModel = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::Sphere(fovFarZ, ColorF32{0.5, 0.7, 1.0}))
            .setShader(m_shaders.skydome)
            .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                        .setDepth(GraphicsDepthOptions::Default3D().setWriteMask(false))
            )
            .setCbv10AndLater({skydome_b4})
        };

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1024, 1024}, 32, ColorF32{0.9}, ColorF32{0.3});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::TexturePlane(groundPlaneTexture, Float2{1024.0f, 1024.0f}))
            .setShader(m_shaders.model)
        }.uploadWorldMatrix(Mat4x4::Translate({0.0f, groundPositionY, 0.0f}));

        m_playerDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(m_models.playerModel)
            .setShader(m_shaders.phong)
            .setCbv10AndLater({m_cb.phongLight})
        };

        m_playerPose.position.y = groundPositionY + 15.0f;

        m_mountainDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(m_models.mountainModel)
            .setShader(m_shaders.phong)
            .setCbv10AndLater({m_cb.phongLight})
        };

        InitFsr2();
    }

    // -----------------------------------------------

    FfxFsr2ContextDescription m_initializationParameters = {};

    FfxFsr2Context m_context;

    ComPtr<ID3D12Resource> m_motionVectorTex;

    ComPtr<ID3D12Resource> m_upscaledOutputTex;

    TextureDrawer m_upscaledOutputDrawer{};

    RenderTarget m_inputRT{{.size = Scene::Size() * 0.5, .clearColor = ColorF32{0.0f, 1.0f}}};

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
        m_playerDrawer.uploadWorldMatrix(m_playerPose.getMatrix()).draw();

        if (not KeyShift.pressed())
        {
            m_camera.transformBySimpleInput();
        }
        else
        {
            const auto matrix = m_playerPose.getMatrix();
            const auto forward = matrix.forward();
            const auto right = matrix.right();
            const auto up = matrix.up();

            const auto moveInput = SimpleInput::GetPlayerMovement3D();
            constexpr auto speed = 10.0f;
            m_playerPose.position += forward * moveInput.z * speed * System::DeltaTime();
            m_playerPose.position += right * moveInput.x * speed * System::DeltaTime();
            m_playerPose.position += up * moveInput.y * speed * System::DeltaTime();

            const auto rotateInput = SimpleInput::GetCameraRotation();
            constexpr auto rotationSpeed = 2.0f;
            m_playerPose.rotation *= Quaternion::RotateY(rotateInput.x * rotationSpeed * System::DeltaTime());
        }

        Graphics3D::SetViewMatrix(m_camera.viewMatrix());

        {
            Float2 jitter_ndc = 2.0f * m_jitterOffset / m_inputRT.size();

            // jitter_ndc *= 5.0f; // FIXME
            // std::cout  << "jitter ndc: " << jitter_ndc.x << ", " << jitter_ndc.y << "\n";

            const Mat4x4 jitterMat = Mat4x4::Translate({jitter_ndc.x, -jitter_ndc.y, 0.0f});

            m_projectionMat = Mat4x4::PerspectiveFov(
                75.0_deg,
                Scene::Size().horizontalAspectRatio(),
                fovNearZ,
                fovFarZ
            );

            m_projectionMat = m_projectionMat * jitterMat;

            Graphics3D::SetProjectionMatrix(m_projectionMat);
        }

        m_cb.phongLight->lightDirection = Float3{0.3f, -1.0f, 0.3f}.normalized();
        m_cb.phongLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        m_cb.phongLight->eyePosition = m_camera.eyePosition();
        m_cb.phongLight->ambientColor = Float3{0.3f, 0.35f, 0.35f};

        m_cb.phongLight.upload();

        m_planeLight->lightDirection = Float3(0.5f, -1.0f, 0.5f).normalized();
        m_planeLight->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_planeLight.upload();

        // -----------------------------------------------

        m_skydomeModel.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        m_groundPlaneDrawer.draw();

        m_mountainDrawer.uploadWorldMatrix(Mat4x4::Scale(Float3{5.0})).draw();

        m_playerDrawer.draw();
    }

    void Update()
    {
        {
            auto bind = m_inputRT.scopedBind();
            draw3D();
        }

        DrawFsr2();

        {
            ImGui::Begin("Camera");

            ImGui::Text("Eye Position: (%.2f, %.2f, %.2f)",
                        m_camera.eyePosition().x,
                        m_camera.eyePosition().y,
                        m_camera.eyePosition().z);

            const auto targetPosition = m_camera.targetPosition();
            ImGui::Text("Target Position: (%.2f, %.2f, %.2f)",
                        targetPosition.x,
                        targetPosition.y,
                        targetPosition.z);

            ImGui::Text("Light Direction: (%.2f, %.2f, %.2f)",
                        m_cb.phongLight->lightDirection.x,
                        m_cb.phongLight->lightDirection.y,
                        m_cb.phongLight->lightDirection.z);

            ImGui::Separator();

            if (ImGui::Button("Reset Camera"))
            {
                resetCamera();
            }

            ImGui::End();
        }

        {
            ImGui::Begin("System Settings");

            static bool s_sleep{};;
            ImGui::Checkbox("Sleep", &s_sleep);

            if (s_sleep)
            {
                System::Sleep(500);
            }

            ImGui::End();
        }
    }

private:
    void resetCamera()
    {
        m_camera.reset(Float3{0.0f, 15.0f, 15.0f});
    }
};

void Demo_FSR2_ST()
{
    Demo_FSR2_ST_impl impl{};

    Scene::RequestResize({1920, 1080});

    while (System::Update())
    {
        impl.Update();
    }
}
