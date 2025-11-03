#pragma once
#include <d3dcommon.h>

#include "ITimestamp.h"

namespace TY
{
    struct ShaderParams
    {
        std::string filepath;
        std::string entryPoint;

        static ShaderParams VS(const std::string& filename);

        static ShaderParams PS(const std::string& filename);

        static ShaderParams CS(const std::string& filename);
    };

    struct Shader_impl;

    class VertexShader
    {
    public:
        VertexShader() = default;

        explicit VertexShader(const ShaderParams& params);

        explicit VertexShader(const std::string& filepath, const std::string& entryPoint)
            : VertexShader{ShaderParams{.filepath = filepath, .entryPoint = entryPoint}}
        {
        }

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        std::shared_ptr<ITimestamp> timestamp() const;

        [[nodiscard]]
        ID3D10Blob* getBlob() const;

        [[nodiscard]]
        std::string getErrorMessage() const;

        [[nodiscard]]
        size_t unique_id() const;

    private:
        std::shared_ptr<Shader_impl> p_impl;
    };

    class PixelShader
    {
    public:
        PixelShader() = default;

        explicit PixelShader(const ShaderParams& params);

        explicit PixelShader(const std::string& filepath, const std::string& entryPoint)
            : PixelShader{ShaderParams{.filepath = filepath, .entryPoint = entryPoint}}
        {
        }

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        std::shared_ptr<ITimestamp> timestamp() const;

        [[nodiscard]]
        ID3D10Blob* getBlob() const;

        [[nodiscard]]
        std::string getErrorMessage() const;

        [[nodiscard]]
        size_t unique_id() const;

    private:
        std::shared_ptr<Shader_impl> p_impl;
    };

    struct GraphicsShader
    {
        VertexShader vs{};
        PixelShader ps{};

        GraphicsShader withVS(const VertexShader& vs_) const;

        GraphicsShader withPS(const PixelShader& ps_) const;

        static GraphicsShader VS_PS(const std::string& filepath);
    };

    class ComputeShader
    {
    public:
        ComputeShader() = default;

        explicit ComputeShader(const ShaderParams& params);

        explicit ComputeShader(const std::string& filename, const std::string& entryPoint)
            : ComputeShader{ShaderParams{.filepath = filename, .entryPoint = entryPoint}}
        {
        }

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        std::shared_ptr<ITimestamp> timestamp() const;

        [[nodiscard]]
        ID3D10Blob* getBlob() const;

        [[nodiscard]]
        std::string getErrorMessage() const;

        [[nodiscard]]
        size_t unique_id() const;

    private:
        std::shared_ptr<Shader_impl> p_impl;
    };
}
