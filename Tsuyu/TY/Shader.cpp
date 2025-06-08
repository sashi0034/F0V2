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

    Shader_impl(const ShaderParams& params, std::string_view target) : m_params(params), m_target(target)
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

        const auto filename = ToUtf16(m_params.filename);
        const auto compileResult = D3DCompileFromFile(
            filename.c_str(),
            nullptr,
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
    PixelShader::PixelShader(const ShaderParams& params)
        : p_impl{std::make_shared<Shader_impl>(params, "ps_5_0"sv)}
    {
#ifdef _DEBUG
        EngineHotReloader::TrackAsset(p_impl, {FileWatcher(p_impl->m_params.filename).timestamp()});
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

    VertexShader::VertexShader(const ShaderParams& params)
        : p_impl{std::make_shared<Shader_impl>(params, "vs_5_0"sv)}
    {
#ifdef _DEBUG
        EngineHotReloader::TrackAsset(p_impl, {FileWatcher(p_impl->m_params.filename).timestamp()});
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
}
