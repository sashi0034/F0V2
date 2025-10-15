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

        Array<ConstantBufferCore> cbv10AndLater{};

        Array<ShaderResourceType> srv10AndLater{};

        GenericModelDrawerParams& setModel(const std::shared_ptr<IGenericModelBuffer>& model_);

        GenericModelDrawerParams& setVertexInput(const Array<VertexInputElement>& vertexInput_);

        GenericModelDrawerParams& setShader(const GraphicsShader& shader_);

        GenericModelDrawerParams& setOptions(const GraphicsOptions& options_);

        GenericModelDrawerParams& setCbv10AndLater(const Array<ConstantBufferCore>& cbv);

        GenericModelDrawerParams& setSrv10AndLater(const Array<ShaderResourceType>& srv);
    };

    class GenericModelDrawer
    {
    public:
        GenericModelDrawer() = default;

        GenericModelDrawer(const GenericModelDrawerParams& params);

        const GenericModelDrawer& uploadWorldMatrix(const Mat4x4& worldMatrix) const;

        void draw() const;

        void draw(int materialIndexOfCbv10AndLater) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
