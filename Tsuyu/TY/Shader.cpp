#include "pch.h"
#include "Shader.h"

#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")

#include "AssertObject.h"
#include "FileWatcher.h"
#include "Logger.h"
#include "System.h"
#include "Utils.h"
#include "detail/EngineHotReloader.h"
#include "detail/RenderContext_singleton.h"
#include "detail/IEngineHotReloadable.h"

using namespace TY;
using namespace TY::detail;

struct TY::Shader_impl : IEngineHotReloadable
{
    uint64_t m_timestamp{};
    ComPtr<ID3DBlob> m_shaderBlob{};
    ComPtr<ID3DBlob> m_errorBlob{};
    ShaderParams m_params{};
    std::string_view m_target{};
    std::vector<D3D_SHADER_MACRO> m_macros{};

    Shader_impl(const ShaderParams& params, std::string_view target, const std::vector<D3D_SHADER_MACRO>& macros = {})
        : m_params(params),
          m_target(target),
          m_macros(std::move(macros))
    {
        Shader_impl::HotReload();
    }

    ~Shader_impl()
    {
        DisposeRenderResource();
    }

    std::string GetErrorMessage() const
    {
        if (not m_errorBlob || m_errorBlob->GetBufferSize() == 0)
        {
            return "";
        }

        return std::string{static_cast<char*>(m_errorBlob->GetBufferPointer()), m_errorBlob->GetBufferSize() - 1};
    }

    uint64_t timestamp() const override
    {
        return m_timestamp;
    }

    void DisposeRenderResource()
    {
        RenderContext_singleton::SafeDisposeRenderResource(m_shaderBlob);
    }

    void HotReload() override
    {
        m_timestamp = System::FrameCount();
        DisposeRenderResource();

        const std::wstring filepath = ToUtf16(m_params.filepath);

        // DXC 初期化
        ComPtr<IDxcUtils> utils;
        ComPtr<IDxcCompiler3> compiler;
        ComPtr<IDxcIncludeHandler> includeHandler;
        HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
        if (FAILED(hr))
        {
            LogError.writeln(L"Shader: Failed to create DxcUtils.");
            return;
        }

        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        if (FAILED(hr))
        {
            LogError.writeln(L"Shader: Failed to create DxcCompiler.");
            return;
        }

        utils->CreateDefaultIncludeHandler(&includeHandler);

        // ソース読み込み
        ComPtr<IDxcBlobEncoding> sourceBlob;
        hr = utils->LoadFile(filepath.c_str(), nullptr, &sourceBlob);
        if (FAILED(hr))
        {
            LogError.writeln(L"Shader: File not found: " + filepath);
            return;
        }

        DxcBuffer sourceBuffer{};
        sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
        sourceBuffer.Size = sourceBlob->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        // コンパイル引数
        std::wstring entryPointW = ToUtf16(m_params.entryPoint);
        std::wstring targetW = ToUtf16(m_target);

        std::vector<LPCWSTR> args = {
            filepath.c_str(),
            L"-E", entryPointW.c_str(),
            L"-T", targetW.c_str(),
            L"-Zpr", // Row-major matrices
        };

#ifdef defined(_DEBUG)
        args.insert(args.end(), {
                        L"-Zi",
                        L"-Qembed_debug",
                        L"-Od",
                        L"-DDEBUG",
                    });
#else
        args.push_back(L"-O3");
#endif

        // コンパイル実行
        ComPtr<IDxcResult> result;
        hr = compiler->Compile(
            &sourceBuffer,
            args.data(),
            static_cast<uint32_t>(args.size()),
            includeHandler.Get(),
            IID_PPV_ARGS(&result)
        );

        if (FAILED(hr) || result == nullptr)
        {
            LogError.writeln(L"Shader: DXC compilation failed to start: " + filepath + L".");
            return;
        }

        // エラー出力
        ComPtr<IDxcBlobUtf8> errors;
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if (errors && errors->GetStringLength() > 0)
        {
            std::string msg = errors->GetStringPointer();
            LogError.writeln(L"Shader: DXC compilation errors:\n" + ToUtf16(msg));
        }

        HRESULT status;
        result->GetStatus(&status);
        if (FAILED(status))
        {
            LogError.writeln(L"Shader: DXC compilation failed: " + filepath + L".");
            return;
        }

        // 出力 (DXIL Blob)
        ComPtr<IDxcBlob> shaderBlob;
        result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
        if (!shaderBlob)
        {
            LogError.writeln(L"Shader: DXC compilation produced no object: " + filepath + L".");
            return;
        }

        ComPtr<ID3DBlob> dxilBlob;
        shaderBlob.As(&dxilBlob);

        m_shaderBlob = dxilBlob;
    }
};

namespace
{
}

namespace TY
{
    ShaderParams ShaderParams::VS(const std::string& filename)
    {
        return ShaderParams{.filepath = filename, .entryPoint = "VS",};
    }

    ShaderParams ShaderParams::PS(const std::string& filename)
    {
        return ShaderParams{.filepath = filename, .entryPoint = "PS",};
    }

    ShaderParams ShaderParams::CS(const std::string& filename)
    {
        return ShaderParams{.filepath = filename, .entryPoint = "CS",};
    }

    VertexShader::VertexShader(const ShaderParams& params)
        : p_impl{std::make_shared<Shader_impl>(params, "vs_6_0"sv)}
    {
#if defined(_DEBUG)
        EngineHotReloader::TrackAsset(p_impl, {FileWatcher(p_impl->m_params.filepath).timestamp()});
#endif
    }

    bool VertexShader::isEmpty() const
    {
        return p_impl == nullptr || p_impl->m_shaderBlob == nullptr;
    }

    std::shared_ptr<ITimestamp> VertexShader::timestamp() const
    {
        if (not p_impl) return InvalidTimestamp;
        return p_impl;
    }

    ID3D10Blob* VertexShader::getBlob() const
    {
        return p_impl ? p_impl->m_shaderBlob.Get() : nullptr;
    }

    std::string VertexShader::getErrorMessage() const
    {
        return p_impl ? p_impl->GetErrorMessage() : "";
    }

    size_t VertexShader::unique_id() const
    {
        return reinterpret_cast<size_t>(p_impl.get());
    }

    PixelShader::PixelShader(const ShaderParams& params)
        : p_impl{std::make_shared<Shader_impl>(params, "ps_6_0"sv)}
    {
#if defined(_DEBUG)
        EngineHotReloader::TrackAsset(p_impl, {FileWatcher(p_impl->m_params.filepath).timestamp()});
#endif
    }

    bool PixelShader::isEmpty() const
    {
        return p_impl == nullptr || p_impl->m_shaderBlob == nullptr;
    }

    std::shared_ptr<ITimestamp> PixelShader::timestamp() const
    {
        if (not p_impl) return InvalidTimestamp;
        return p_impl;
    }

    ID3D10Blob* PixelShader::getBlob() const
    {
        return p_impl ? p_impl->m_shaderBlob.Get() : nullptr;
    }

    std::string PixelShader::getErrorMessage() const
    {
        return p_impl ? p_impl->GetErrorMessage() : "";
    }

    size_t PixelShader::unique_id() const
    {
        return reinterpret_cast<size_t>(p_impl.get());
    }

    GraphicsShader GraphicsShader::withVS(const VertexShader& vs_) const
    {
        auto clone = *this;
        clone.vs = vs_;
        return clone;
    }

    GraphicsShader GraphicsShader::withPS(const PixelShader& ps_) const
    {
        auto clone = *this;
        clone.ps = ps_;
        return clone;
    }

    GraphicsShader GraphicsShader::VS_PS(const std::string& filepath)
    {
        return GraphicsShader{
            VertexShader{ShaderParams::VS(filepath)},
            PixelShader{ShaderParams::PS(filepath)}
        };
    }

    ComputeShader::ComputeShader(const ShaderParams& params)
        : p_impl(std::make_shared<Shader_impl>(params, "cs_6_0"sv))
    {
#if defined(_DEBUG)
        EngineHotReloader::TrackAsset(p_impl, {FileWatcher(p_impl->m_params.filepath).timestamp()});
#endif
    }

    bool ComputeShader::isEmpty() const
    {
        return p_impl == nullptr || p_impl->m_shaderBlob == nullptr;
    }

    std::shared_ptr<ITimestamp> ComputeShader::timestamp() const
    {
        if (not p_impl) return InvalidTimestamp;
        return p_impl;
    }

    ID3D10Blob* ComputeShader::getBlob() const
    {
        return p_impl ? p_impl->m_shaderBlob.Get() : nullptr;
    }

    std::string ComputeShader::getErrorMessage() const
    {
        return p_impl ? p_impl->GetErrorMessage() : "";
    }

    size_t ComputeShader::unique_id() const
    {
        return reinterpret_cast<size_t>(p_impl.get());
    }
}
