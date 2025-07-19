#pragma once
#include "CbvSrvUav.h"
#include "ConstantBufferUploader.h"
#include "GenericModelDrawer.h"
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

        Array<ConstantBufferUploaderCore> cbv10AndLater{};

        Array<ShaderResourceType> srv10AndLater{};

        ModelDrawerParams& loadModel(const std::string& filename);

        ModelDrawerParams& setModel(const ModelBuffer& model_);

        ModelDrawerParams& setShader(const VertexShader& vs_, const PixelShader& ps_);

        ModelDrawerParams& setShader(const GraphicsShader& shader_);

        ModelDrawerParams& setOptions(const GraphicsOptions& options_);

        ModelDrawerParams& setCbv10AndLater(const Array<ConstantBufferUploaderCore>& cbv);

        ModelDrawerParams& setSrv10AndLater(const Array<ShaderResourceType>& srv);
    };

    class ModelDrawer
    {
    public:
        ModelDrawer() = default;

        ModelDrawer(const ModelDrawerParams& params);

        const ModelDrawer& uploadWorldMatrix(const Mat4x4& worldMatrix) const;

        void draw() const;

        void draw(int materialIndexOfCbv10AndLater) const;

    private:
        GenericModelDrawer m_impl{};
    };
}
