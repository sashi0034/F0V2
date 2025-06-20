#include "pch.h"
#include "EngineCacheContext.h"

#include "TY/StructuredBufferUploader.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    struct StructuredBufferUploaderCache
    {
        std::weak_ptr<IGpgpuBuffer> lifetime;
        StructuredBufferUploader structuredBuffer;
    };

    struct StructuredBufferTransferCache
    {
        std::weak_ptr<IGpgpuBuffer> lifetime;
        StructuredBufferTransfer structuredBuffer;
    };
}

struct EngineCacheContextImpl
{
    std::unordered_map<IGpgpuBuffer*, StructuredBufferUploaderCache> m_structuredBufferUploaderCache{};

    std::unordered_map<IGpgpuBuffer*, StructuredBufferTransferCache> m_structuredBufferTransferCache{};

    void Update()
    {
        cleanUp(m_structuredBufferUploaderCache);
        cleanUp(m_structuredBufferTransferCache);
    }

private:
    template <typename Container>
    void cleanUp(Container& cache)
    {
        for (auto it = cache.begin(); it != cache.end();)
        {
            if (it->second.lifetime.expired())
            {
                it = cache.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
};

namespace
{
    EngineCacheContextImpl s_cacheContext{};
}

namespace TY::detail
{
    void EngineCacheContext::Update()
    {
        s_cacheContext.Update();
    }

    void EngineCacheContext::Shutdown()
    {
        s_cacheContext = {};
    }

    StructuredBufferUploader EngineCacheContext::FetchStructuredBufferUploader(
        const std::shared_ptr<IGpgpuBuffer>& key)
    {
        if (not key)
        {
            return {};
        }

        auto& cache = s_cacheContext.m_structuredBufferUploaderCache;
        if (const auto it = cache.find(key.get()); it != cache.end())
        {
            return it->second.structuredBuffer;
        }

        const auto& cache2 = s_cacheContext.m_structuredBufferTransferCache;
        if (const auto it2 = cache2.find(key.get()); it2 != cache2.end())
        {
            return it2->second.structuredBuffer;
        }

        StructuredBufferUploader uploader{StructuredBufferTransferParams::From(key)};
        cache[key.get()] = {key, uploader};
        return uploader;
    }

    StructuredBufferTransfer EngineCacheContext::FetchStructuredBufferTransfer(const std::shared_ptr<IGpgpuBuffer>& key)
    {
        if (not key)
        {
            return {};
        }

        auto& cache = s_cacheContext.m_structuredBufferTransferCache;

        if (const auto it = cache.find(key.get()); it != cache.end())
        {
            return it->second.structuredBuffer;
        }

        StructuredBufferTransfer transfer{StructuredBufferTransferParams::From(key)};
        cache[key.get()] = {key, transfer};
        return transfer;
    }
}
