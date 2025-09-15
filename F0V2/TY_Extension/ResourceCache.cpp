#include "pch.h"
#include "ResourceCache.h"

#include "TY/ModelLoader.h"

using namespace TY;

namespace
{
    struct CommonResourceCache : IInlineComponent
    {
        GraphicsShaderCache_t graphicsShaderCache{};
        ModelBufferCache_t modelBufferCache{};
    };

    InlineComponent<CommonResourceCache> s_commonResourceCache{};
}

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

    GraphicsShaderCache_t& GraphicsShaderCache()
    {
        return s_commonResourceCache->graphicsShaderCache;
    }

    GraphicsShader GraphicsShaderCache(
        const std::string& path,
        const std::function<GraphicsShader(const std::string& path)>& loader)
    {
        return GraphicsShaderCache().fetch(path, loader);
    }

    ModelBufferCache_t& ModelBufferCache()
    {
        return s_commonResourceCache->modelBufferCache;
    }

    ModelBuffer ModelBufferCache(
        const std::string& path,
        const std::function<ModelBuffer(const std::string& path)>& loader)
    {
        return ModelBufferCache().fetch(path, loader);
    }
}
