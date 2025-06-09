#pragma once
#include "Array.h"
#include "ConstantBufferUploader.h"
#include "Shader.h"

namespace TY
{
    struct ModelParams
    {
        std::string filename;
        PixelShader ps;
        VertexShader vs;
        std::optional<ConstantBufferUploader_impl> cb2{};
    };

    class Model
    {
    public:
        Model() = default;

        Model(const ModelParams& params);

        Model(const std::string& filename, const PixelShader& ps, const VertexShader& vs)
            : Model{ModelParams{filename, ps, vs}}
        {
        }

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
