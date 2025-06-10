#pragma once
#include "ConstantBufferUploader.h"
#include "ModelData.h"
#include "Shader.h"

namespace TY
{
    struct ModelParams
    {
        ModelData data;
        PixelShader ps;
        VertexShader vs;
        ConstantBufferUploader_impl cb2{Empty};

        ModelParams& loadData(const std::string& filename);

        ModelParams& setData(const ModelData& data_);

        ModelParams& setShaders(const PixelShader& ps_, const VertexShader& vs_);

        ModelParams& setCB2(const ConstantBufferUploader_impl& cb2_);
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
