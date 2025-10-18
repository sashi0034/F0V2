#include "pch.h"
#include "EngineRenderContext.h"

#include <dxgi1_6.h>
#include <dxgidebug.h>

#include "CommandListManager.h"
#include "EngineStateContext.h"
#include "EngineWindow.h"
#include "GpuMemoryUsage.h"
#include "TY/Logger.h"
#include "TY/Mat3x2.h"

using namespace TY;
using namespace TY::detail;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace
{
    constexpr ColorF32 defaultClearColor = {0.5f, 0.5f, 0.5f, 1.0f};

    constexpr Size defaultFrameBufferSize = {1920, 1080};

    void enableDebugLayer()
    {
        ComPtr<ID3D12Debug> debugLayer = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
        {
            debugLayer->EnableDebugLayer();
        }
    }

    void reportLiveObjects()
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(
                DXGI_DEBUG_ALL,
                DXGI_DEBUG_RLO_ALL
            );
        }
    }

    bool isNull(const RenderResource& renderResource)
    {
        return std::visit([](auto&& arg) { return arg == nullptr; }, renderResource);
    }
}

struct EngineRenderContextImpl
{
    bool m_valid{};

    Point m_frameBufferSize{defaultFrameBufferSize};
    ColorF32 m_clearColor{defaultClearColor};

    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory6> m_dxgiFactory;
    ComPtr<IDXGIAdapter> m_adapter;
    D3D_FEATURE_LEVEL m_featureLevel{};

    GpuMemoryUsage m_gpuMemoryUsage{};

    CommandListManager m_drawCommandList{};
    // CommandList m_copyCommandList{};
    // CommandList m_computeCommandList{};

    ComPtr<IDXGISwapChain4> m_swapChain{};

    std::array<RenderTarget, EngineRenderContext::FrameBufferCount> m_backBuffers{};
    ScopedRenderTarget m_scopedBackBuffer{};

    Mat3x2 m_windowToFrameBuffer{};
    Mat3x2 m_frameBufferToWindow{};

    std::optional<Size> m_wantsFrameBufferSize{};

    // bool m_wasFullscreen{};

    std::optional<bool> m_wantsFullscreen{};

    ConstantBuffer<SceneState3D_b0> m_sceneState3D{Empty};

    std::array<Array<RenderResource>, EngineRenderContext::FrameBufferCount> m_disposedRenderResources{};

    // Copy のフラッシュとともに加算
    size_t m_flushTimestamp{};

    void Init()
    {
#ifdef _DEBUG
        enableDebugLayer();
#endif

        // デバッグフラグ有効で DXGI ファクトリを生成
        if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()))))
        {
            // 失敗した場合、デバッグフラグ無効で DXGI ファクトリを生成
            if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()))))
            {
                LogError.writeln("CreateDXGIFactory2() failed");
                return;
            }
        }

        // 利用可能なアダプタを取得
        std::vector<ComPtr<IDXGIAdapter>> availableAdapters{};
        {
            ComPtr<IDXGIAdapter> tmp = nullptr;
            for (int i = 0; m_dxgiFactory->EnumAdapters(i, tmp.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                availableAdapters.push_back(tmp);
            }
        }

        // 最適なアダプタを選択
        for (const auto& adapter : availableAdapters)
        {
            DXGI_ADAPTER_DESC desc = {};
            adapter->GetDesc(&desc);
            std::wstring strDesc = desc.Description;
            if (strDesc.find(L"NVIDIA") != std::string::npos)
            {
                m_adapter = adapter;
                break;
            }
        }

        if (not m_adapter)
        {
            LogError.writeln("No suitable adapter found");
            return;
        }

        // Direct3D デバイスの初期化
        static constexpr std::array levels = {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        for (const auto level : levels)
        {
            if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), level, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()))))
            {
                m_featureLevel = level;
                break;
            }
        }

        LogInfo.writeln(std::format("Direct3D feature level: {:08x}", static_cast<int>(m_featureLevel)));

        m_gpuMemoryUsage = GpuMemoryUsage{m_dxgiFactory.Get(), m_device->GetAdapterLuid()};

        // コマンドリストの作成
        m_drawCommandList = CommandListManager{CommandListType::Draw};

        // m_copyCommandList = CommandList{CommandListType::Copy};

        // m_computeCommandList = CommandList{CommandListType::Compute};

        // スワップチェインの設定
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = m_frameBufferSize.x;
        swapchainDesc.Height = m_frameBufferSize.y;
        swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.Stereo = false;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
        swapchainDesc.BufferCount = EngineRenderContext::FrameBufferCount;
        swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        if (const auto hr = m_dxgiFactory->CreateSwapChainForHwnd(
                m_drawCommandList.getCommandQueue(),
                EngineWindow::Handle(),
                &swapchainDesc,
                nullptr,
                nullptr,
                reinterpret_cast<IDXGISwapChain1**>(m_swapChain.GetAddressOf()));
            FAILED(hr))
        {
            LogError.writeln(std::format("CreateSwapChainForHwnd() failed with error code: {}", static_cast<int>(hr)));
            return;
        }

        // バックバッファ作成
        setupBackBuffers();

        // 共通コンスタントバッファの初期化
        m_sceneState3D = ConstantBuffer<SceneState3D_b0>{1};

        m_valid = true;
    }

    void NewFrame()
    {
        // リサイズ制御
        checkRecreateSwapchain();

        const auto backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        m_backBuffers[backBufferIndex].setViewport(calculateViewportRect());

        m_windowToFrameBuffer = calculateWindowToFrameBuffer();
        m_frameBufferToWindow = m_windowToFrameBuffer.inverse();

        // バックバッファを設定
        m_scopedBackBuffer = m_backBuffers[backBufferIndex].scopedBind();
    }

    void Render()
    {
        // コンスタントバッファのアップロード
        {
            SceneState3D_b0 b{};
            b.projectionMatrix = EngineStateContext::GetProjectionMatrix();
            b.viewMatrix = EngineStateContext::GetViewMatrix();
            m_sceneState3D.upload(b);
        }

        // バックバッファ反映
        m_scopedBackBuffer.dispose();

        // コマンドリストの実行
        SubmitCommand();

        // フリップ
        m_swapChain->Present(1, 0);
    }

    Array<RenderResource>& CurrentDisposedRenderResources()
    {
        const size_t index = m_flushTimestamp % EngineRenderContext::FrameBufferCount;
        return m_disposedRenderResources[index];
    }

    // void FlushComputeCommandSync()
    // {
    //     m_copyCommandList.CloseAndFlushAfter(m_drawCommandList);
    //     m_computeCommandList.CloseAndFlushAfter(m_copyCommandList);
    //
    //     m_flushTimestamp++;
    //
    //     CurrentDisposedRenderResources().clear();
    //
    //     m_computeCommandList.WaitLastFlush();
    // }

    void SubmitCommand()
    {
        m_drawCommandList.closeAndAdvance();

        m_flushTimestamp++;

        CurrentDisposedRenderResources().clear();
    }

    CommandListManager& GetCommandList(CommandListType type)
    {
        switch (type)
        {
        case CommandListType::Draw:
            return m_drawCommandList;
        // case CommandListType::Copy:
        //     return m_copyCommandList;
        // case CommandListType::Compute:
        //     return m_computeCommandList;
        default:
            assert(false);
            return m_drawCommandList;
        }
    }

    void OnShutdown()
    {
        SubmitCommand();

        m_drawCommandList.waitLastCommandList();
    }

private:
    void setupBackBuffers()
    {
        for (int i = 0; i < EngineRenderContext::FrameBufferCount; ++i)
        {
            TextureHandle backBuffer{};
            m_swapChain->GetBuffer(i, IID_PPV_ARGS(backBuffer.assignResourceAddress(D3D12_RESOURCE_STATE_PRESENT)));

            m_backBuffers[i] =
                RenderTargetParams{}
                .setRtvAndClearColor_unsafe(backBuffer, m_clearColor);
        }
    }

    RectF calculateViewportRect() const
    {
        const auto windowSize = EngineWindow::GetSize();

        if (windowSize == m_frameBufferSize)
        {
            return RectF{0.0f, 0.0f, windowSize};
        }

        const auto windowRatio = windowSize.horizontalAspectRatio();

        const auto sceneRatio = m_frameBufferSize.horizontalAspectRatio();

        if (sceneRatio > windowRatio)
        {
            // 縦長のウィンドウ
            const float width = static_cast<float>(m_frameBufferSize.x);
            const float windowHeightInScene = windowSize.y * static_cast<float>(m_frameBufferSize.x) / windowSize.x;
            const float height = m_frameBufferSize.y * m_frameBufferSize.y / windowHeightInScene;
            return RectF{0.0f, (m_frameBufferSize.y - height) * 0.5f, width, height};
        }
        else
        {
            // 横長のウィンドウ
            const float height = static_cast<float>(m_frameBufferSize.y);
            const float windowWidthInScene = windowSize.x * static_cast<float>(m_frameBufferSize.y) / windowSize.y;
            const float width = m_frameBufferSize.x * m_frameBufferSize.x / windowWidthInScene;
            return RectF{(m_frameBufferSize.x - width) * 0.5f, 0.0f, width, height};
        }
    }

    Mat3x2 calculateWindowToFrameBuffer() const
    {
        const Float2 windowSize = EngineWindow::GetSize().cast<Float2>();
        const float frameBufferScaling = (Float2(m_frameBufferSize) / windowSize).maxComponent();
        const Float2 windowSizeInScene = windowSize * frameBufferScaling;
        return Mat3x2::Identity()
               .scaled(Float2{frameBufferScaling, frameBufferScaling})
               .translated((m_frameBufferSize.cast<Float2>() - windowSizeInScene) * 0.5f);
    }

    // void toggleFullscreen()
    // {
    //     if (not m_wantsFullscreen.has_value())
    //     {
    //         return;
    //     }
    //
    //     const bool shouldFullscreen = m_wantsFullscreen.value();
    //
    //     m_wantsFullscreen = {};
    //
    //     ComPtr<IDXGIOutput> output;
    //     m_swapChain->GetContainingOutput(&output);
    //
    //     DXGI_OUTPUT_DESC desc;
    //     output->GetDesc(&desc);
    //     const auto& targetMode = desc.DesktopCoordinates; // 表示解像度取得
    //
    //     m_swapChain->SetFullscreenState(shouldFullscreen, nullptr);
    //
    //     recreateSwapChain(shouldFullscreen
    //                           ? Size{targetMode.right - targetMode.left, targetMode.bottom - targetMode.top}
    //                           : m_frameBufferSize);
    // }

    void checkRecreateSwapchain()
    {
        std::optional<Size> newSize{};

        // フルスクリーン制御
        if (m_wantsFullscreen)
        {
            newSize = m_frameBufferSize;

            m_wantsFullscreen = {};
        }

        // リサイズのリクエスト対応
        if (m_wantsFrameBufferSize.has_value())
        {
            if (m_wantsFrameBufferSize != m_frameBufferSize)
            {
                newSize = m_wantsFrameBufferSize;
            }

            m_wantsFrameBufferSize = {};
        }

        if (newSize.has_value())
        {
            // リサイズしたものを再作成
            recreateSwapChain(*newSize);
        }
    }

    void recreateSwapChain(const Size& newSize)
    {
        assert(not m_scopedBackBuffer.isActive());
        assert(RenderTarget::Current().isEmpty());

        SubmitCommand();

        m_drawCommandList.waitLastCommandList();

        m_backBuffers = {};

        for (auto& rsc : m_disposedRenderResources)
        {
            rsc.clear();
        }

        if (const auto hr = m_swapChain->ResizeBuffers(
                EngineRenderContext::FrameBufferCount,
                newSize.x,
                newSize.y,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
            FAILED(hr))
        {
            LogError.writeln(std::format(
                "IDXGISwapChain::ResizeBuffers(): Failed with error code: {}",
                static_cast<int>(hr)));
            return;
        }

        setupBackBuffers();

        m_frameBufferSize = newSize;
    }
};

namespace
{
    EngineRenderContextImpl s_renderContext{};
}

namespace TY::detail
{
    void EngineRenderContext::Init()
    {
        s_renderContext.Init();
    }

    void EngineRenderContext::NewFrame()
    {
        s_renderContext.NewFrame();
    }

    void EngineRenderContext::Render()
    {
        s_renderContext.Render();
    }

    void EngineRenderContext::Shutdown()
    {
        s_renderContext.OnShutdown();

        s_renderContext = {};

        // All COM objects should be released before this call
        reportLiveObjects();
    }

    ID3D12Device* EngineRenderContext::GetDevice()
    {
        assert(s_renderContext.m_device);
        return s_renderContext.m_device.Get();
    }

    ID3D12GraphicsCommandList* EngineRenderContext::TargetCommandList()
    {
        return s_renderContext.m_drawCommandList.getCommandList(); // TODO: draw or compute
    }

    void EngineRenderContext::FlushComputeCommandSync()
    {
        // s_renderContext.FlushComputeCommandSync();
    }

    void EngineRenderContext::RequestFrameBufferSize(Size frameBufferSize)
    {
        s_renderContext.m_wantsFrameBufferSize = frameBufferSize;
    }

    void EngineRenderContext::RequestFullscreen(bool fullscreen)
    {
        s_renderContext.m_wantsFullscreen = fullscreen;
    }

    bool EngineRenderContext::IsFullscreen()
    {
        BOOL fullscreen;
        s_renderContext.m_swapChain->GetFullscreenState(&fullscreen, nullptr);
        return fullscreen != FALSE;
    }

    Size EngineRenderContext::FrameBufferSize()
    {
        return s_renderContext.m_frameBufferSize;
    }

    Mat3x2 EngineRenderContext::WindowToFrameBuffer()
    {
        return s_renderContext.m_windowToFrameBuffer;
    }

    Mat3x2 EngineRenderContext::FrameBufferToWindow()
    {
        return s_renderContext.m_frameBufferToWindow;
    }

    ConstantBuffer<SceneState3D_b0> EngineRenderContext::GetSceneState3D_CB0()
    {
        return s_renderContext.m_sceneState3D;
    }

    void EngineRenderContext::SafeDisposeRenderResource(const RenderResource& renderResource)
    {
        if (not isNull(renderResource))
        {
            s_renderContext.CurrentDisposedRenderResources().push_back(renderResource);
        }
    }

    size_t EngineRenderContext::GetFlushTimestamp()
    {
        return s_renderContext.m_flushTimestamp;
    }

    IGpuMemoryUsage& EngineRenderContext::GpuMemoryUsage()
    {
        return s_renderContext.m_gpuMemoryUsage;
    }
}
