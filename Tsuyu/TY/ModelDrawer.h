#pragma once
#include "CbvSrvUav.h"
#include "ConstantBufferArray.h"
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

        Array<ConstantBufferArrayImpl> cbv10AndLater{};

        Array<ShaderResourceType> srv10AndLater{};

        int dynamicCbvCount{};

        ModelDrawerParams& loadModel(const std::string& filename);

        ModelDrawerParams& setModel(const ModelBuffer& model_);

        ModelDrawerParams& setShader(const VertexShader& vs_, const PixelShader& ps_);

        ModelDrawerParams& setShader(const GraphicsShader& shader_);

        ModelDrawerParams& setOptions(const GraphicsOptions& options_);

        ModelDrawerParams& setCbv10AndLater(const Array<ConstantBufferArrayImpl>& cbv);

        ModelDrawerParams& setSrv10AndLater(const Array<ShaderResourceType>& srv);

        ModelDrawerParams& setDynamicCbvCount(int count);
    };

    class ModelDrawer
    {
    public:
        ModelDrawer() = default;

        ModelDrawer(const ModelDrawerParams& params);

        const ModelDrawer& uploadWorldMatrix(const Mat4x4& worldMatrix) const;

        [[nodiscard]]
        RootParameterIndex mapDynamicCbvIndex(int index) const;

        void draw() const;

    private:
        GenericModelDrawer m_impl{};
    };
}
