#include "pch.h"
#include "ResourceCache.h"

#include "TY/ModelLoader.h"

using namespace TY;

namespace
{
    struct CommonResourceCache : IInlineComponent
    {
        VertexShaderCache::cache_type vertexShaderCache{};
        PixelShaderCache::cache_type pixelShaderCache{};
        GraphicsShaderCache::cache_type graphicsShaderCache{};
        ComputeShaderCache::cache_type computeShaderCache{};
        ModelBufferCache::cache_type modelBufferCache{};
        SoundAudioCache::cache_type soundAudioCache{};
        MusicAudioCache::cache_type musicAudioCache{};
    };

    InlineComponent<CommonResourceCache> s_commonResourceCache{};
}

namespace TY
{
    VertexShader VertexShaderCache::LoadDefault(const std::string& path)
    {
        return VertexShader(ShaderParams::VS(path));
    }

    VertexShaderCache::cache_type& VertexShaderCache::operator()() const
    {
        return s_commonResourceCache->vertexShaderCache;
    }

    PixelShader PixelShaderCache::LoadDefault(const std::string& path)
    {
        return PixelShader(ShaderParams::PS(path));
    }

    PixelShaderCache::cache_type& PixelShaderCache::operator()() const
    {
        return s_commonResourceCache->pixelShaderCache;
    }

    GraphicsShader GraphicsShaderCache::LoadDefault(const std::string& path)
    {
        return GraphicsShader::VS_PS(path);
    }

    GraphicsShaderCache::cache_type& GraphicsShaderCache::operator()() const
    {
        return s_commonResourceCache->graphicsShaderCache;
    }

    ComputeShader ComputeShaderCache::LoadDefault(const std::string& path)
    {
        return ComputeShader(ShaderParams::CS(path));
    }

    ComputeShaderCache::cache_type& ComputeShaderCache::operator()() const
    {
        return s_commonResourceCache->computeShaderCache;
    }

    ModelBuffer ModelBufferCache::LoadDefault(const std::string& path)
    {
        return ModelBuffer(ModelLoader::Load(path));
    }

    ModelBufferCache::cache_type& ModelBufferCache::operator()() const
    {
        return s_commonResourceCache->modelBufferCache;
    }

    SoundAudio SoundAudioCache::LoadDefault(const std::string& path)
    {
        return SoundAudio(path);
    }

    SoundAudioCache::cache_type& SoundAudioCache::operator()() const
    {
        return s_commonResourceCache->soundAudioCache;
    }

    MusicAudio MusicAudioCache::LoadDefault(const std::string& path)
    {
        return MusicAudio(path);
    }

    MusicAudioCache::cache_type& MusicAudioCache::operator()() const
    {
        return s_commonResourceCache->musicAudioCache;
    }
}
