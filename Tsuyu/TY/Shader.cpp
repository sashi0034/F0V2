#include "pch.h"
#include "Shader.h"

#include <d3dcompiler.h>

#include "AssertObject.h"
#include "FileWatcher.h"
#include "Logger.h"
#include "System.h"
#include "Utils.h"
#include "detail/EngineHotReloader.h"
#include "detail/IEngineHotReloadable.h"

using namespace TY;
using namespace TY::detail;

struct TY::Shader_impl : IEngineHotReloadable
{
    uint64_t m_timestamp{};
    ComPtr<ID3DBlob> shaderBlob{};
    ComPtr<ID3DBlob> errorBlob{};
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

    std::string GetErrorMessage() const
    {
        if (not errorBlob) return "";
        return std::string{static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize()};
    }

    uint64_t timestamp() const override
    {
        return m_timestamp;
    }

    void HotReload() override
    {
        m_timestamp = System::FrameCount();

        const auto filepath = ToUtf16(m_params.filepath);
        const auto compileResult = D3DCompileFromFile(
            filepath.c_str(),
            m_macros.empty() ? nullptr : m_macros.data(),
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            m_params.entryPoint.c_str(),
            m_target.data(),
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // FIXME
            0,
            shaderBlob.ReleaseAndGetAddressOf(),
            errorBlob.ReleaseAndGetAddressOf()
        );

        if (SUCCEEDED(compileResult)) return;
        // -----------------------------------------------

        if (shaderBlob != nullptr) shaderBlob->Release();

        std::wstring message = L"failed to compile shader: ";
        if (compileResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            message += L"file not found";
        }
        else
        {
            message += ToUtf16(GetErrorMessage());
        }

        LogError.writeln(message);
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
        : p_impl{std::make_shared<Shader_impl>(params, "vs_5_0"sv)}
    {
#ifdef _DEBUG
        EngineHotReloader::TrackAsset(p_impl, {FileWatcher(p_impl->m_params.filepath).timestamp()});
#endif
    }

    bool VertexShader::isEmpty() const
    {
        return p_impl == nullptr || p_impl->shaderBlob == nullptr;
    }

    std::shared_ptr<ITimestamp> VertexShader::timestamp() const
    {
        if (not p_impl) return InvalidTimestamp;
        return p_impl;
    }

    ID3D10Blob* VertexShader::getBlob() const
    {
        return p_impl ? p_impl->shaderBlob.Get() : nullptr;
    }

    size_t VertexShader::unique_id() const
    {
        return reinterpret_cast<size_t>(p_impl.get());
    }

    PixelShader::PixelShader(const ShaderParams& params)
        : p_impl{std::make_shared<Shader_impl>(params, "ps_5_0"sv)}
    {
#ifdef _DEBUG
        EngineHotReloader::TrackAsset(p_impl, {FileWatcher(p_impl->m_params.filepath).timestamp()});
#endif
    }

    bool PixelShader::isEmpty() const
    {
        return p_impl == nullptr || p_impl->shaderBlob == nullptr;
    }

    std::shared_ptr<ITimestamp> PixelShader::timestamp() const
    {
        if (not p_impl) return InvalidTimestamp;
        return p_impl;
    }

    ID3D10Blob* PixelShader::getBlob() const
    {
        return p_impl ? p_impl->shaderBlob.Get() : nullptr;
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
        : p_impl(std::make_shared<Shader_impl>(params, "cs_5_0"sv))
    {
#ifdef _DEBUG
        EngineHotReloader::TrackAsset(p_impl, {FileWatcher(p_impl->m_params.filepath).timestamp()});
#endif
    }

    bool ComputeShader::isEmpty() const
    {
        return p_impl == nullptr || p_impl->shaderBlob == nullptr;
    }

    std::shared_ptr<ITimestamp> ComputeShader::timestamp() const
    {
        if (not p_impl) return InvalidTimestamp;
        return p_impl;
    }

    ID3D10Blob* ComputeShader::getBlob() const
    {
        return p_impl ? p_impl->shaderBlob.Get() : nullptr;
    }

    size_t ComputeShader::unique_id() const
    {
        return reinterpret_cast<size_t>(p_impl.get());
    }
}
