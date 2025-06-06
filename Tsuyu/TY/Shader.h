#pragma once
#include <d3dcommon.h>

#include "ITimestamp.h"

namespace TY
{
    struct ShaderParams
    {
        std::string filename;
        std::string entryPoint;
    };

    struct Shader_impl;

    class ScopedPixelShader;

    class PixelShader
    {
    public:
        PixelShader() = default;

        explicit PixelShader(const ShaderParams& params);

        explicit PixelShader(const std::string& filename, const std::string& entryPoint)
            : PixelShader{ShaderParams{.filename = filename, .entryPoint = entryPoint}}
        {
        }

        [[nodiscard]] bool isEmpty() const;

        [[nodiscard]] std::shared_ptr<ITimestamp> timestamp() const;

        [[nodiscard]] ID3D10Blob* getBlob() const;

    private:
        std::shared_ptr<Shader_impl> p_impl;
    };

    class VertexShader
    {
    public:
        VertexShader() = default;

        explicit VertexShader(const ShaderParams& params);

        explicit VertexShader(const std::string& filename, const std::string& entryPoint)
            : VertexShader{ShaderParams{.filename = filename, .entryPoint = entryPoint}}
        {
        }

        [[nodiscard]] bool isEmpty() const;

        [[nodiscard]] std::shared_ptr<ITimestamp> timestamp() const;

        [[nodiscard]] ID3D10Blob* getBlob() const;

    private:
        std::shared_ptr<Shader_impl> p_impl;
    };

    // class ScopedShader : Uncopyable
    // {
    // public:
    //     explicit ScopedShader(const PixelShader& pixelShader, const VertexShader& vertexShader);
    //
    //     ~ScopedShader();
    //
    // private:
    //     size_t m_timestamp{};
    // };
}
