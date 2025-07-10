#pragma once
#include "ConstantBufferUploader.h"
#include "GraphicsOptions.h"
#include "Mat4x4.h"
#include "ModelBuffer.h"
#include "Shader.h"

namespace TY
{
    struct ModelDrawerParams
    {
        ModelBuffer model;
        PixelShader ps;
        VertexShader vs;
        ConstantBufferUploader_impl cb4{Empty};
        GraphicsOptions options{GraphicsOptions::Default3D()};

        ModelDrawerParams& loadModel(const std::string& filename);

        ModelDrawerParams& setModel(const ModelBuffer& data_);

        ModelDrawerParams& setShaders(const PixelShader& ps_, const VertexShader& vs_);

        ModelDrawerParams& setShaders(const GraphicsShader& shader);

        ModelDrawerParams& setCB4(const ConstantBufferUploader_impl& cb2_);

        ModelDrawerParams& setOptions(const GraphicsOptions& options_);
    };

    class ModelDrawer
    {
    public:
        ModelDrawer() = default;

        ModelDrawer(const ModelDrawerParams& params);

        const ModelDrawer& uploadWorldMatrix(const Mat4x4& worldMatrix) const;

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
