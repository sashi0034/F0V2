#pragma once
#include "ConstantBufferUploader.h"
#include "ModelData.h"
#include "Shader.h"

namespace TY
{
    struct ModelDrawerParams
    {
        ModelData data;
        PixelShader ps;
        VertexShader vs;
        ConstantBufferUploader_impl cb2{Empty};

        ModelDrawerParams& loadData(const std::string& filename);

        ModelDrawerParams& setData(const ModelData& data_);

        ModelDrawerParams& setShaders(const PixelShader& ps_, const VertexShader& vs_);

        ModelDrawerParams& setCB2(const ConstantBufferUploader_impl& cb2_);
    };

    class ModelDrawer
    {
    public:
        ModelDrawer() = default;

        ModelDrawer(const ModelDrawerParams& params);

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
