#pragma once
#include "CbSrUa.h"
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

        GraphicsOptions options{GraphicsOptions::Default3D()};

        ConstantBufferUploader_impl cb4{Empty};
        ConstantBufferUploader_impl cb5{Empty};
        ConstantBufferUploader_impl cb6{Empty};
        ConstantBufferUploader_impl cb7{Empty};

        ShaderResourceType sr1{};
        ShaderResourceType sr2{};
        ShaderResourceType sr3{};
        ShaderResourceType sr4{};
        ShaderResourceType sr5{};
        ShaderResourceType sr6{};
        ShaderResourceType sr7{};

        ModelDrawerParams& loadModel(const std::string& filename);

        ModelDrawerParams& setModel(const ModelBuffer& data_);

        ModelDrawerParams& setShaders(const PixelShader& ps_, const VertexShader& vs_);

        ModelDrawerParams& setShaders(const GraphicsShader& shader);

        ModelDrawerParams& setOptions(const GraphicsOptions& options_);

        ModelDrawerParams& setCB4(const ConstantBufferUploader_impl& cb);
        ModelDrawerParams& setCB5(const ConstantBufferUploader_impl& cb);
        ModelDrawerParams& setCB6(const ConstantBufferUploader_impl& cb);
        ModelDrawerParams& setCB7(const ConstantBufferUploader_impl& cb);

        ModelDrawerParams& setSR1(const ShaderResourceType& sr);
        ModelDrawerParams& setSR2(const ShaderResourceType& sr);
        ModelDrawerParams& setSR3(const ShaderResourceType& sr);
        ModelDrawerParams& setSR4(const ShaderResourceType& sr);
        ModelDrawerParams& setSR5(const ShaderResourceType& sr);
        ModelDrawerParams& setSR6(const ShaderResourceType& sr);
        ModelDrawerParams& setSR7(const ShaderResourceType& sr);
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
