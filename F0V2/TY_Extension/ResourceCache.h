#pragma once
#include "TY/Audio.h"
#include "TY/InlineComponent.h"
#include "TY/ModelBuffer.h"
#include "TY/Shader.h"

namespace TY
{
    template <typename T, T (*defaultLoader)(const std::string& path)>
    class ResourceCache
    {
    public:
        ResourceCache() = default;

        using resource_type = T;

        void upload(const std::string& path, const T& resource)
        {
            m_cache[path] = resource;
        }

        /// @brief 登録されたローダー関数を使ってリソースを取得する
        /// @param path リソースのパス
        /// @param loader ローダー関数
        /// @return リソース。キーが登録されていない場合はデフォルトコンストラクタで生成されたオブジェクトを返す
        T fetch(const std::string& path, const std::function<T(const std::string& path)>& loader)
        {
            if (const auto it = m_cache.find(path); it != m_cache.end())
            {
                return it->second;
            }

            auto rsc = loader ? loader(path) : defaultLoader(path);
            m_cache[path] = rsc;

            return rsc;
        }

        T operator()(const std::string& path)
        {
            return fetch(path, defaultLoader);
        }

        T operator ()(const std::string& path, const std::function<T(const std::string& path)>& loader)
        {
            return fetch(path, loader);
        }

        void unregisterAll()
        {
            m_cache.clear();
        }

    private:
        std::unordered_map<std::string, T> m_cache{};
    };

    struct VertexShaderCache
    {
        static VertexShader LoadDefault(const std::string& path);

        using cache_type = ResourceCache<VertexShader, LoadDefault>;

        cache_type& operator ()() const;
    };

    struct PixelShaderCache
    {
        static PixelShader LoadDefault(const std::string& path);

        using cache_type = ResourceCache<PixelShader, LoadDefault>;

        cache_type& operator ()() const;
    };

    struct GraphicsShaderCache
    {
        static GraphicsShader LoadDefault(const std::string& path);

        using cache_type = ResourceCache<GraphicsShader, LoadDefault>;

        cache_type& operator ()() const;
    };

    struct ComputeShaderCache
    {
        static ComputeShader LoadDefault(const std::string& path);

        using cache_type = ResourceCache<ComputeShader, LoadDefault>;

        cache_type& operator ()() const;
    };

    struct ModelBufferCache
    {
        static ModelBuffer LoadDefault(const std::string& path);

        using cache_type = ResourceCache<ModelBuffer, LoadDefault>;

        cache_type& operator ()() const;
    };

    struct FontObjectCache
    {
        static ModelBuffer LoadDefault(const std::string& path);

        using cache_type = ResourceCache<ModelBuffer, LoadDefault>;

        cache_type& operator ()() const;
    };

    struct SoundAudioCache
    {
        static SoundAudio LoadDefault(const std::string& path);

        using cache_type = ResourceCache<SoundAudio, LoadDefault>;

        cache_type& operator ()() const;
    };

    struct MusicAudioCache
    {
        static MusicAudio LoadDefault(const std::string& path);

        using cache_type = ResourceCache<MusicAudio, LoadDefault>;

        cache_type& operator ()() const;
    };
}
