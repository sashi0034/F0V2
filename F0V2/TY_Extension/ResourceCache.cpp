#include "pch.h"
#include "ResourceCache.h"

#include "TY/ModelLoader.h"

using namespace TY;

namespace
{
    struct CommonResourceCache : IInlineComponent
    {
        GraphicsShaderCache::cache_type graphicsShaderCache{};
        ModelBufferCache::cache_type modelBufferCache{};
    };

    InlineComponent<CommonResourceCache> s_commonResourceCache{};
}

namespace TY
{
    ModelBuffer detail::DefaultModelBufferCacheLoader(const std::string& path)
    {
        return ModelBuffer(ModelLoader::Load(path));
    }

    GraphicsShader GraphicsShaderCache::DefaultLoad(const std::string& path)
    {
        return GraphicsShader::VS_PS(path);
    }

    GraphicsShaderCache::cache_type& GraphicsShaderCache::operator()() const
    {
        return s_commonResourceCache->graphicsShaderCache;
    }

    ModelBuffer ModelBufferCache::DefaultLoad(const std::string& path)
    {
        return detail::DefaultModelBufferCacheLoader(path);
    }

    ModelBufferCache::cache_type& ModelBufferCache::operator()() const
    {
        return s_commonResourceCache->modelBufferCache;
    }
}
