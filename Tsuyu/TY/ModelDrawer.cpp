#include "pch.h"
#include "ModelDrawer.h"

#include "Array.h"
#include "ConstantBuffer.h"
#include "Mat4x4.h"
#include "ModelLoader.h"
#include "detail/DescriptorHeap.h"
#include "detail/GraphicsPipelineState.h"

using namespace TY;
using namespace TY::detail;

namespace TY
{
    ModelDrawerParams& ModelDrawerParams::loadModel(const std::string& filename)
    {
        model = ModelLoader::Load(filename);
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setModel(const ModelBuffer& model_)
    {
        model = model_;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setShader(const VertexShader& vs_, const PixelShader& ps_)
    {
        shader.vs = vs_;
        shader.ps = ps_;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setShader(const GraphicsShader& shader_)
    {
        shader.ps = shader_.ps;
        shader.vs = shader_.vs;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setOptions(const GraphicsOptions& options_)
    {
        options = options_;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setCbv10AndLater(const Array<ConstantBufferImpl>& cbv)
    {
        cbv10AndLater = cbv;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setSrv10AndLater(const Array<ShaderResourceType>& srv)
    {
        srv10AndLater = srv;
        return *this;
    }

    ModelDrawer::ModelDrawer(const ModelDrawerParams& params)
    {
        m_impl = GenericModelDrawer{
            GenericModelDrawerParams{
                .model = params.model.asGeneric(),
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT}
                },
                .shader = params.shader,
                .options = params.options,
                .cbv10AndLater = params.cbv10AndLater,
                .srv10AndLater = params.srv10AndLater
            }
        };
    }

    const ModelDrawer& ModelDrawer::uploadWorldMatrix(const Mat4x4& worldMatrix) const
    {
        (void)m_impl.uploadWorldMatrix(worldMatrix);
        return *this;
    }

    void ModelDrawer::draw() const
    {
        m_impl.draw();
    }
}
