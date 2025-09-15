#pragma once
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

    namespace detail
    {
        GraphicsShader DefaultGraphicsShaderCacheLoader(const std::string& path);

        ModelBuffer DefaultModelBufferCacheLoader(const std::string& path);
    }

    using GraphicsShaderCache_t = ResourceCache<GraphicsShader, detail::DefaultGraphicsShaderCacheLoader>;

    GraphicsShaderCache_t& GraphicsShaderCache();

    GraphicsShader GraphicsShaderCache(
        const std::string& path,
        const std::function<GraphicsShader(const std::string& path)>& loader = nullptr);

    using ModelBufferCache_t = ResourceCache<ModelBuffer, detail::DefaultModelBufferCacheLoader>;

    ModelBufferCache_t& ModelBufferCache();

    ModelBuffer ModelBufferCache(
        const std::string& path,
        const std::function<ModelBuffer(const std::string& path)>& loader = nullptr);
}
