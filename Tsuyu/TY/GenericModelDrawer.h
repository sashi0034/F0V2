#pragma once
#include "GenericModelBuffer.h"
#include "TY/GraphicsOptions.h"
#include "TY/Mat4x4.h"
#include "TY/Shader.h"
#include "TY/VertexInputElement.h"

namespace TY
{
    struct GenericModelDrawerParams
    {
        std::shared_ptr<IGenericModelBuffer> model;

        Array<VertexInputElement> vertexInput;

        GraphicsShader shader;

        GraphicsOptions options{GraphicsOptions::Default3D()};

        DescriptorList<ConstantBufferImpl> cbv10AndLater{};

        DescriptorList<ShaderResourceType> srv10AndLater{};

        int dynamicCbvCount{};

        int dynamicSrvCount{};

        GenericModelDrawerParams& setModel(const std::shared_ptr<IGenericModelBuffer>& model_);

        GenericModelDrawerParams& setVertexInput(const Array<VertexInputElement>& vertexInput_);

        GenericModelDrawerParams& setShader(const GraphicsShader& shader_);

        GenericModelDrawerParams& setOptions(const GraphicsOptions& options_);

        GenericModelDrawerParams& setCbv10AndLater(const DescriptorList<ConstantBufferImpl>& cbv);

        GenericModelDrawerParams& setSrv10AndLater(const DescriptorList<ShaderResourceType>& srv);

        GenericModelDrawerParams& setDynamicCbvCount(int count);

        GenericModelDrawerParams& setDynamicSrvCount(int count);
    };

    class GenericModelDrawer
    {
    public:
        GenericModelDrawer() = default;

        GenericModelDrawer(const GenericModelDrawerParams& params);

        // TODO: Rename
        const GenericModelDrawer& uploadWorldMatrix(const Mat4x4& worldMatrix) const;

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
