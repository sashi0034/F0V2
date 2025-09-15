#include "pch.h"
#include "ResourceCache.h"

#include "TY/ModelLoader.h"

namespace TY
{
    GraphicsShader detail::DefaultGraphicsShaderCacheLoader(const std::string& path)
    {
        return GraphicsShader::VS_PS(path);
    }

    ModelBuffer detail::DefaultModelBufferCacheLoader(const std::string& path)
    {
        return ModelBuffer(ModelLoader::Load(path));
    }
}
