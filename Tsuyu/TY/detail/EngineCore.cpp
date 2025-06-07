#include "pch.h"
#include "EngineCore.h"

#include <cassert>

#include "Windows.h"

#include "TY/Value2D.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include "CommandList.h"
#include "EngineHotReloader.h"
#include "EngineImGUI.h"
#include "EngineKeyboard.h"
#include "EnginePresetAsset.h"
#include "EngineWindow.h"
#include "TY/Array.h"
#include "TY/AssertObject.h"
#include "TY/Color.h"
#include "TY/EngineTimer.h"
#include "TY/RenderTarget.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

namespace
{
    using namespace TY;
    using namespace TY::detail;

    using namespace std::string_view_literals;

    void enableDebugLayer()
    {
        ID3D12Debug* debugLayer = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
        {
            debugLayer->EnableDebugLayer();
            debugLayer->Release();
        }
    }

    constexpr ColorF32 defaultClearColor = {0.5f, 0.5f, 0.5f, 1.0f};

    constexpr Size defaultSceneSize = {1280, 720};

    struct Impl
    {
        Point m_sceneSize{defaultSceneSize};
        ColorF32 m_clearColor{defaultClearColor};

        ID3D12Device* m_device{};
        IDXGIFactory6* m_dxgiFactory{};
        IDXGIAdapter* m_adapter{};
        D3D_FEATURE_LEVEL m_featureLevel{};

        // FIXME: グローバルオブジェクトは ComPtr にしなくていいかも

        bool m_inFrame{};

        CommandList m_commandList{};
        CommandList m_copyCommandList{};

        ComPtr<IDXGISwapChain4> m_swapChain{};

        RenderTarget m_backBuffer{};
        ScopedRenderTarget m_scopedBackBuffer{};

        Array<std::weak_ptr<IEngineUpdatable>> m_updatableList{};

        void Init()
        {
            EngineWindow::Init();
#ifdef _DEBUG
            enableDebugLayer();
#endif

            // デバッグフラグ有効で DXGI ファクトリを生成
            if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_dxgiFactory))))
            {
                // 失敗した場合、デバッグフラグ無効で DXGI ファクトリを生成
                if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_dxgiFactory))))
                {
                    throw std::runtime_error("failed to create DXGI Factory");
                }
            }

            // 利用可能なアダプタを取得
            std::vector<IDXGIAdapter*> availableAdapters{};
            {
                IDXGIAdapter* tmp = nullptr;
                for (int i = 0; m_dxgiFactory->EnumAdapters(i, &tmp) != DXGI_ERROR_NOT_FOUND; ++i)
                {
                    availableAdapters.push_back(tmp);
                }
            }

            // 最適なアダプタを選択
            for (const auto adapter : availableAdapters)
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
                throw std::runtime_error("failed to select adapter");
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
                if (D3D12CreateDevice(m_adapter, level, IID_PPV_ARGS(&m_device)) == S_OK)
                {
                    m_featureLevel = level;
                    break;
                }
            }

            // コマンドリストの作成
            m_commandList = CommandList{CommandListType::Direct};

            m_copyCommandList = CommandList{CommandListType::Copy};

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
            AssertWin32{"failed to create swap chain"sv}
                | m_dxgiFactory->CreateSwapChainForHwnd(
                    m_commandList.GetCommandQueue(),
                    EngineWindow::Handle(),
                    &swapchainDesc,
                    nullptr,
                    nullptr,
                    reinterpret_cast<IDXGISwapChain1**>(m_swapChain.GetAddressOf())
                );

            // バックバッファ作成
            m_backBuffer = RenderTarget{
                {
                    .bufferCount = static_cast<int>(swapchainDesc.BufferCount),
                    .size = m_sceneSize,
                    .clearColor = m_clearColor,
                },
                m_swapChain.Get()
            };

            // ウィンドウ表示
            EngineWindow::Show();

            // タイマーの初期化
            EngineTimer.Reset();

            // プリセットの初期化
            EnginePresetAsset::Init();

            // ImGUI 初期化
            EngineImGui::Init();
        }

        void BeginFrame()
        {
            m_inFrame = true;

            // バックバッファを設定
            const auto backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
            m_scopedBackBuffer = m_backBuffer.scopedBind(backBufferIndex);

            // ウィンドウの更新
            EngineWindow::Update();

            // タイマーの更新
            EngineTimer.Tick();

            // アップデータの更新
            for (auto& updatable : m_updatableList)
            {
                if (const auto updatablePtr = updatable.lock())
                {
                    updatablePtr->Update();
                }
            }

            // ホットリローダの更新
            EngineHotReloader::Update();

            // 入力情報の更新
            EngineKeyboard::Update();

            // ImGUI フレーム開始
            EngineImGui::NewFrame();
        }

        void EndFrame()
        {
            // ImGUI 描画
            EngineImGui::Render();

            // バックバッファ反映
            m_scopedBackBuffer.dispose();

            // コマンドリストの実行
            m_copyCommandList.CloseAndFlush();
            m_commandList.CloseAndFlush();

            // フリップ
            m_swapChain->Present(1, 0);

            m_inFrame = false;
        }

        void Destroy()
        {
            EngineWindow::Shutdown();

            m_copyCommandList.CloseAndFlush();
            m_commandList.CloseAndFlush();

            EngineHotReloader::Shutdown();

            EnginePresetAsset::Shutdown();

            EngineImGui::Shutdown();
        }

    private:
    } s_engineCore{};
}

namespace TY
{
    void EngineCore_impl::Init() const
    {
        s_engineCore.Init();
    }

    bool EngineCore_impl::IsInFrame() const
    {
        return s_engineCore.m_inFrame;
    }

    void EngineCore_impl::BeginFrame() const
    {
        s_engineCore.BeginFrame();
    }

    void EngineCore_impl::EndFrame() const
    {
        s_engineCore.EndFrame();
    }

    void EngineCore_impl::Destroy() const
    {
        s_engineCore.Destroy();
    }

    const RenderTarget& EngineCore_impl::GetBackBuffer() const
    {
        return s_engineCore.m_backBuffer;
    }

    ID3D12Device* EngineCore_impl::GetDevice() const
    {
        assert(s_engineCore.m_device);
        return s_engineCore.m_device;
    }

    ID3D12GraphicsCommandList* EngineCore_impl::GetCommandList() const
    {
        assert(s_engineCore.m_commandList.GetCommandList());
        return s_engineCore.m_commandList.GetCommandList();
    }

    ID3D12GraphicsCommandList* EngineCore_impl::GetCopyCommandList() const
    {
        assert(s_engineCore.m_copyCommandList.GetCommandList());
        return s_engineCore.m_copyCommandList.GetCommandList();
    }

    Size EngineCore_impl::GetSceneSize() const
    {
        return s_engineCore.m_sceneSize;
    }

    void EngineCore_impl::AddUpdatable(const std::weak_ptr<IEngineUpdatable>& updatable) const
    {
        s_engineCore.m_updatableList.push_back(updatable);
    }
}
