#include "pch.h"
#include "EngineRenderContext.h"

#include <dxgi1_5.h>
#include <dxgi1_6.h>

#include "CommandList.h"
#include "EngineWindow.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace
{
    constexpr ColorF32 defaultClearColor = {0.5f, 0.5f, 0.5f, 1.0f};

    constexpr Size defaultSceneSize = {1280, 720};

    void enableDebugLayer()
    {
        ID3D12Debug* debugLayer = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
        {
            debugLayer->EnableDebugLayer();
            debugLayer->Release();
        }
    }
}

struct EngineRenderContextImpl
{
    bool m_valid{};

    Point m_sceneSize{defaultSceneSize};
    ColorF32 m_clearColor{defaultClearColor};

    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory6> m_dxgiFactory;
    ComPtr<IDXGIAdapter> m_adapter;
    D3D_FEATURE_LEVEL m_featureLevel{};

    CommandList m_commandList{};
    CommandList m_copyCommandList{};
    CommandList m_computeCommandList{};

    ComPtr<IDXGISwapChain4> m_swapChain{};

    RenderTarget m_backBuffer{};
    ScopedRenderTarget m_scopedBackBuffer{};

    Array<CommandListType> m_commandTargetStack{};

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

        // コマンドリストの作成
        m_commandList = CommandList{CommandListType::Direct};

        m_copyCommandList = CommandList{CommandListType::Copy};

        m_computeCommandList = CommandList{CommandListType::Compute};

        // スワップチェインの設定
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = m_sceneSize.x;
        swapchainDesc.Height = m_sceneSize.y;
        swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.Stereo = false;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
        swapchainDesc.BufferCount = 2;
        swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        if (const auto hr = m_dxgiFactory->CreateSwapChainForHwnd(
                m_commandList.GetCommandQueue(),
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
        m_backBuffer = RenderTarget{
            {
                .bufferCount = static_cast<int>(swapchainDesc.BufferCount),
                .size = m_sceneSize,
                .clearColor = m_clearColor,
            },
            m_swapChain.Get()
        };

        m_valid = true;
    }

    void NewFrame()
    {
        m_backBuffer.setScissorRect(getScissorRect());

        // バックバッファを設定
        const auto backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        m_scopedBackBuffer = m_backBuffer.scopedBind(backBufferIndex);
    }

    void Render()
    {
        assert(m_commandTargetStack.empty());

        // バックバッファ反映
        m_scopedBackBuffer.dispose();

        // コマンドリストの実行
        FlushCommandLists();

        // フリップ
        m_swapChain->Present(1, 0);
    }

    void FlushCommandLists()
    {
        m_computeCommandList.CloseAndFlush();
        m_copyCommandList.CloseAndFlush();
        m_commandList.CloseAndFlush();
    }

    CommandList& getActiveCommandList()
    {
        if (m_commandTargetStack.empty())
        {
            return m_commandList;
        }

        switch (m_commandTargetStack.back())
        {
        case CommandListType::Direct:
            return m_commandList;
        case CommandListType::Copy:
            return m_copyCommandList;
        case CommandListType::Compute:
            return m_computeCommandList;
        default:
            assert(false);
            return m_commandList;
        }
    }

    void OnShutdown()
    {
        FlushCommandLists();
    }

private:
    RectF getScissorRect() const
    {
        const auto windowSize = EngineWindow::WindowSize();

        if (windowSize == m_sceneSize)
        {
            return RectF{0.0f, 0.0f, windowSize};
        }

        const auto windowRatio = windowSize.horizontalAspectRatio();

        const auto sceneRatio = m_sceneSize.horizontalAspectRatio();

        if (sceneRatio > windowRatio)
        {
            // 縦長のウィンドウ
            const auto width = static_cast<float>(m_sceneSize.x);
            const auto windowHeight = windowSize.y * static_cast<float>(m_sceneSize.x) / windowSize.x;
            const auto height = static_cast<float>(m_sceneSize.y) * static_cast<float>(m_sceneSize.y) / windowHeight;
            return RectF{0.0f, (m_sceneSize.y - height) / 2.0f, width, height};
        }
        else
        {
            // 横長のウィンドウ
            const auto height = static_cast<float>(m_sceneSize.y);
            const auto windowWidth = windowSize.x * static_cast<float>(m_sceneSize.y) / windowSize.y;
            const auto width = static_cast<float>(m_sceneSize.x) * static_cast<float>(m_sceneSize.x) / windowWidth;

            return RectF{(m_sceneSize.x - width) / 2.0f, 0.0f, width, height};
        }
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
    }

    const RenderTarget& EngineRenderContext::GetBackBuffer()
    {
        return s_renderContext.m_backBuffer;
    }

    ID3D12Device* EngineRenderContext::GetDevice()
    {
        assert(s_renderContext.m_device);
        return s_renderContext.m_device.Get();
    }

    ScopedDefer EngineRenderContext::ScopedCommandTarget(CommandListType type)
    {
        s_renderContext.m_commandTargetStack.push_back(type);
        return ScopedDefer{
            [type]()
            {
                if (not s_renderContext.m_commandTargetStack.empty())
                {
                    assert(s_renderContext.m_commandTargetStack.back() == type);
                    s_renderContext.m_commandTargetStack.pop_back();
                }
                else
                {
                    assert(false);
                }
            }
        };
    }

    CommandListType EngineRenderContext::ActiveCommandTarget()
    {
        if (s_renderContext.m_commandTargetStack.empty())
        {
            return CommandListType::Direct;
        }

        return s_renderContext.m_commandTargetStack.back();
    }

    ID3D12GraphicsCommandList* EngineRenderContext::ActiveCommandList()
    {
        return s_renderContext.getActiveCommandList().GetCommandList();
    }

    void EngineRenderContext::FlushActiveCommandList()
    {
        s_renderContext.getActiveCommandList().CloseAndFlush();
    }

    Size EngineRenderContext::GetSceneSize()
    {
        return s_renderContext.m_sceneSize;
    }
}
