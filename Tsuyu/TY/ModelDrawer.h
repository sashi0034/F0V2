#pragma once
#include "ConstantBufferUploader.h"
#include "Mat4x4.h"
#include "ModelBuffer.h"
#include "ModelData.h"
#include "Shader.h"

namespace TY
{
    struct ModelDrawerParams
    {
        ModelBuffer model;
        PixelShader ps;
        VertexShader vs;
        ConstantBufferUploader_impl cb4{Empty};

        ModelDrawerParams& loadModel(const std::string& filename);

        ModelDrawerParams& setModel(const ModelBuffer& data_);

        ModelDrawerParams& setShaders(const PixelShader& ps_, const VertexShader& vs_);

        ModelDrawerParams& setCB4(const ConstantBufferUploader_impl& cb2_);
    };

    class ModelDrawer
    {
    public:
        ModelDrawer() = default;

        ModelDrawer(const ModelDrawerParams& params);

        void draw(const Mat4x4& worldMatrix) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
