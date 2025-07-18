#pragma once
#include "CbvSrvUav.h"
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

        GraphicsShader shader;

        GraphicsOptions options{GraphicsOptions::Default3D()};

        Array<ConstantBufferUploader_impl> cb4AndLater{Empty};

        Array<ShaderResourceType> sr1AndLater{};

        ModelDrawerParams& loadModel(const std::string& filename);

        ModelDrawerParams& setModel(const ModelBuffer& data_);

        ModelDrawerParams& setShader(const VertexShader& vs_, const PixelShader& ps_);

        ModelDrawerParams& setShader(const GraphicsShader& shader_);

        ModelDrawerParams& setOptions(const GraphicsOptions& options_);

        ModelDrawerParams& setCbv4AndLater(const Array<ConstantBufferUploader_impl>& cbv);

        ModelDrawerParams& setSrv1AndLater(const Array<ShaderResourceType>& srv);
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
