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
        ConstantBufferUploader_impl cb2{Empty};
    };

    class Model
    {
    public:
        Model() = default;

        Model(const ModelParams& params);

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
