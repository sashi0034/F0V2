#pragma once
#include "TY_Extension/ResourceCache.h"

namespace TY
{
    template <typename CacheFunctor>
    // using CacheFunctor = GraphicsShaderCache; // for coding
    class ResourcePathWrapper
    {
    public:
        static inline CacheFunctor Cache{};

        using cache_type = CacheFunctor::cache_type;

        using resource_type = cache_type::resource_type;

        ResourcePathWrapper() = default;

        ResourcePathWrapper(std::string path) : m_path(std::move(path))
        {
        }

        const std::string& path() const
        {
            return m_path;
        }

        resource_type fetchResource() const
        {
            return GetCache().fetch(m_path, nullptr);
        }

        operator resource_type() const
        {
            return fetchResource();
        }

        void uploadResource(const resource_type& resource) const
        {
            GetCache().upload(m_path, resource);
        }

        static cache_type& GetCache()
        {
            return Cache();
        }

    private:
        std::string m_path;
    };

    using VertexShaderPathWrapper = ResourcePathWrapper<VertexShaderCache>;

    using PixelShaderPathWrapper = ResourcePathWrapper<PixelShaderCache>;

    using GraphicsShaderPathWrapper = ResourcePathWrapper<GraphicsShaderCache>;

    using ComputeShaderPathWrapper = ResourcePathWrapper<ComputeShaderCache>;

    using ModelPathWrapper = ResourcePathWrapper<ModelBufferCache>;
}
